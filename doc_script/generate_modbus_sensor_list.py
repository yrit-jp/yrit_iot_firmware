"""
Modbus Device Documentation Generator

Extracts device metadata from modbus_manager.cpp and generates documentation
in multiple formats (JSON, Markdown, AsciiDoc).
"""

import re
import json
import argparse
import os
from pathlib import Path
from typing import Dict, List, Optional
from collections import defaultdict
from abc import ABC, abstractmethod


# ============================================================================
# Path Resolution Utilities
# ============================================================================

class PathResolver:
    """Resolve input file paths relative to project structure."""
    
    @staticmethod
    def find_project_root(start_path: Path = None) -> Optional[Path]:
        """
        Find project root by looking for characteristic files/directories.
        
        Args:
            start_path: Starting directory (default: current directory)
            
        Returns:
            Path to project root, or None if not found
        """
        if start_path is None:
            start_path = Path.cwd()
        
        current = start_path.resolve()
        
        # Look for project markers (main.ino, src directory, etc.)
        markers = ['main.ino', 'yrit_iot_firmware.ino', '_yrit_iot_firmware.ino']
        
        # Search up to 3 levels
        for _ in range(3):
            # Check if any marker exists
            for marker in markers:
                if (current / marker).exists():
                    return current
            
            # Check if src directory exists
            if (current / 'src').is_dir():
                return current
            
            # Move up one level
            parent = current.parent
            if parent == current:  # Reached root
                break
            current = parent
        
        return None
    
    @staticmethod
    def resolve_input_path(input_path: str) -> str:
        """
        Resolve input file path, searching in project structure if needed.
        
        Args:
            input_path: Input path (absolute or relative)
            
        Returns:
            Resolved absolute path
            
        Raises:
            FileNotFoundError: If file cannot be found
        """
        input_path_obj = Path(input_path)
        
        # If absolute path or file exists in current directory, use it
        if input_path_obj.is_absolute() or input_path_obj.exists():
            if input_path_obj.exists():
                return str(input_path_obj.resolve())
            else:
                raise FileNotFoundError(f"File not found: {input_path}")
        
        # Try to find project root
        project_root = PathResolver.find_project_root()
        
        if project_root:
            # Try common locations relative to project root
            search_paths = [
                project_root / input_path,
                project_root / 'src' / 'modbus' / input_path,
                project_root / 'src' / 'modbus' / 'modbus_manager.cpp',
            ]
            
            for path in search_paths:
                if path.exists():
                    return str(path.resolve())
        
        # Try relative to current directory
        if input_path_obj.exists():
            return str(input_path_obj.resolve())
        
        raise FileNotFoundError(
            f"File not found: {input_path}\n"
            f"Searched in:\n"
            f"  - Current directory\n"
            f"  - Project root{f': {project_root}' if project_root else ' (not found)'}\n"
            f"  - src/modbus/ directory"
        )
    
    @staticmethod
    def resolve_output_path(output_path: str, default_name: str) -> str:
        """
        Resolve output file path. If default name is used, output to project_root/notes/.
        
        Args:
            output_path: Output path specified by user
            default_name: Default output name (e.g., 'SUPPORTED_MODBUS_DEVICES')
            
        Returns:
            Resolved output path
        """
        output_path_obj = Path(output_path)
        
        # If user specified a path (not the default), use it as-is
        # Check if it's different from default by comparing the stem (filename without extension)
        if output_path_obj.stem != default_name:
            return output_path
        
        # User is using default name, so output to project_root/notes/
        project_root = PathResolver.find_project_root()
        
        if project_root:
            notes_dir = project_root / 'notes'
            # Create notes directory if it doesn't exist
            notes_dir.mkdir(exist_ok=True)
            return str(notes_dir / default_name)
        
        # No project root found, use current directory
        return output_path


# ============================================================================
# C++ Code Parsing Utilities
# ============================================================================

class CppParser:
    """Utility class for parsing C++ code structures."""
    
    @staticmethod
    def find_matching_brace(text: str, start_pos: int) -> int:
        """
        Find the position of the matching closing brace.
        
        Args:
            text: Source code text
            start_pos: Position of opening brace
            
        Returns:
            Position of matching closing brace, or -1 if not found
        """
        if start_pos >= len(text) or text[start_pos] != '{':
            return -1
        
        count = 1
        for i in range(start_pos + 1, len(text)):
            if text[i] == '{':
                count += 1
            elif text[i] == '}':
                count -= 1
                if count == 0:
                    return i
        return -1
    
    @staticmethod
    def extract_function_body(content: str, function_signature: str) -> Optional[str]:
        """
        Extract the body of a function given its signature pattern.
        
        Args:
            content: Source code content
            function_signature: Regex pattern for function signature
            
        Returns:
            Function body text, or None if not found
        """
        pattern = function_signature + r'\s*\{'
        match = re.search(pattern, content)
        if not match:
            return None
        
        func_start = match.end()
        func_end = CppParser.find_matching_brace(content, func_start - 1)
        if func_end == -1:
            return None
        
        return content[func_start:func_end]


class CommentExtractor:
    """Extract special comments from C++ code."""
    
    PATTERNS = {
        'label': r'//\s*@\s*label:\s*(.+?)(?:\r?\n|$)',
        'url': r'//\s*@\s*url:\s*(.+?)(?:\r?\n|$)',
        'note': r'//\s*@\s*note:\s*(.+?)(?:\r?\n|$)',
    }
    
    @classmethod
    def extract(cls, text: str) -> Dict[str, List[str]]:
        """
        Extract all special comments from text.
        
        Args:
            text: Source code text
            
        Returns:
            Dictionary mapping comment types to lists of values
        """
        comments = defaultdict(list)
        for comment_type, pattern in cls.PATTERNS.items():
            for match in re.finditer(pattern, text):
                comments[comment_type].append(match.group(1).strip())
        return dict(comments)


# ============================================================================
# Device Information Extraction
# ============================================================================

class DeviceInfoExtractor:
    """Extract device information from getDeviceName function."""
    
    @staticmethod
    def extract_if_blocks(text: str) -> List[tuple]:
        """
        Extract all if blocks for different sensors.
        
        Args:
            text: Source code text
            
        Returns:
            List of (sensor_variable, if_body_complete) tuples
        """
        if_blocks = []
        pos = 0
        
        while pos < len(text):
            # Match: if (modbus_sensors[port][id].sensor_name)
            if_match = re.search(r'if\s*\(modbus_sensors\[.*?\]\.(\w+)\)\s*\{', text[pos:])
            if not if_match:
                break
            
            sensor_var = if_match.group(1)
            open_brace_pos = pos + if_match.end() - 1
            close_brace_pos = CppParser.find_matching_brace(text, open_brace_pos)
            
            if close_brace_pos == -1:
                pos += if_match.end()
                continue
            
            # Extract the complete if block body (includes comments inside the if block)
            if_body = text[open_brace_pos + 1:close_brace_pos]
            
            if_blocks.append((sensor_var, if_body))
            pos = close_brace_pos + 1
        
        return if_blocks
    
    @staticmethod
    def extract_device_key(if_body: str) -> Optional[str]:
        """
        Extract device key from if body.
        
        Args:
            if_body: Body of an if block
            
        Returns:
            Device key name, or None if not found
        """
        # Look for default return in switch block
        default_pattern = r'default\s*:\s*return\s*"([^"]+)"'
        default_match = re.search(default_pattern, if_body)
        if default_match:
            return default_match.group(1)
        
        # Look for direct return statement
        direct_return_pattern = r'return\s*"([^"(]+)"'
        direct_returns = re.findall(direct_return_pattern, if_body)
        if direct_returns:
            # Return the shortest name (likely the base device name)
            return min(direct_returns, key=len)
        
        return None
    
    @staticmethod
    def extract_sub_ids(if_body: str) -> List[str]:
        """
        Extract sub_id case values from if body.
        
        Args:
            if_body: Body of an if block
            
        Returns:
            List of sub_id values
        """
        sub_ids = []
        for match in re.finditer(r'case\s+(\d+)\s*:', if_body):
            sub_ids.append(match.group(1))
        return sub_ids if sub_ids else ['0']
    
    @classmethod
    def parse(cls, content: str) -> Dict:
        """
        Parse getDeviceName function and extract device information.
        
        Args:
            content: Source code content
            
        Returns:
            Dictionary mapping device keys to device information
        """
        device_info = {}
        
        # Extract function body
        func_body = CppParser.extract_function_body(
            content, r'String\s+Modbus_Manager::getDeviceName\s*\([^)]+\)'
        )
        if not func_body:
            print("✗ Warning: getDeviceName function not found")
            return device_info
        
        # Extract all if blocks (now includes preceding comments)
        if_blocks = cls.extract_if_blocks(func_body)
        
        # Process each if block
        for sensor_var, if_body in if_blocks:
            # Extract device key
            device_key = cls.extract_device_key(if_body)
            if not device_key:
                continue
            
            # Extract sub_ids
            sub_ids = cls.extract_sub_ids(if_body)
            
            # Extract comments (url, note)
            comments = CommentExtractor.extract(if_body)
            
            # Store device information
            if device_key not in device_info:
                device_info[device_key] = {
                    'name': device_key,
                    'sensor_var': sensor_var,
                    'url': comments.get('url', []),
                    'note': comments.get('note', []),
                    'values': {}
                }
            
            # Initialize sub_ids
            for sub_id in sub_ids:
                if sub_id not in device_info[device_key]['values']:
                    device_info[device_key]['values'][sub_id] = {'label': '', 'unit': ''}
        
        return device_info


class UnitInfoExtractor:
    """Extract unit information from getDeviceUnit function."""
    
    @classmethod
    def parse(cls, content: str) -> Dict:
        """
        Parse getDeviceUnit function and extract unit information.
        
        Args:
            content: Source code content
            
        Returns:
            Dictionary mapping sensor_var to unit information
        """
        unit_info = {}
        
        # Extract function body
        func_body = CppParser.extract_function_body(
            content, r'String\s+Modbus_Manager::getDeviceUnit\s*\([^)]+\)'
        )
        if not func_body:
            print("✗ Warning: getDeviceUnit function not found")
            return unit_info
        
        # Extract all if blocks
        if_blocks = DeviceInfoExtractor.extract_if_blocks(func_body)
        
        # Process each if block
        for sensor_var, if_body in if_blocks:
            # Check for switch(sub_id) block
            sub_switch_match = re.search(r'switch\s*\(\s*sub_id\s*\)\s*\{', if_body)
            
            if sub_switch_match:
                # Extract sub-switch body
                sub_switch_start = sub_switch_match.end() - 1
                sub_switch_end = CppParser.find_matching_brace(if_body, sub_switch_start)
                if sub_switch_end == -1:
                    continue
                
                sub_switch_body = if_body[sub_switch_start + 1:sub_switch_end]
                
                # Extract each sub_id case
                for case_match in re.finditer(r'case\s+(\d+)\s*:(.*?)(?=case\s+\d+|default\s*:|$)', 
                                             sub_switch_body, re.DOTALL):
                    sub_id = case_match.group(1)
                    case_content = case_match.group(2)
                    
                    # Extract comments
                    comments = CommentExtractor.extract(case_content)
                    
                    # Extract return value
                    return_match = re.search(r'return\s+"([^"]*)"', case_content)
                    unit = return_match.group(1) if return_match else ''
                    
                    # Store unit information using sensor_var as key
                    if sensor_var not in unit_info:
                        unit_info[sensor_var] = {}
                    
                    unit_info[sensor_var][sub_id] = {
                        'label': comments.get('label', [''])[0] if comments.get('label') else '',
                        'unit': unit
                    }
            else:
                # Direct return without switch(sub_id) block
                comments = CommentExtractor.extract(if_body)
                label = comments.get('label', [''])[0] if comments.get('label') else ''
                
                return_match = re.search(r'return\s+"([^"]*)"', if_body)
                unit = return_match.group(1) if return_match else ''
                
                # Store for '0' key (default)
                if sensor_var not in unit_info:
                    unit_info[sensor_var] = {}
                
                unit_info[sensor_var]['0'] = {
                    'label': label,
                    'unit': unit
                }
        
        return unit_info


# ============================================================================
# Output Format Generators
# ============================================================================

class BaseFormatter(ABC):
    """Base class for output formatters."""
    
    def format_document(self, data: Dict) -> str:
        """
        Generate complete document.

        Args:
            data: Device data dictionary

        Returns:
            Formatted document string
        """
        # First, generate device sections to calculate line numbers
        device_sections = {}
        for key in sorted(data.keys()):
            device_sections[key] = self.format_device_section(key, data[key])

        # Calculate line numbers for each device section
        line_numbers = self._calculate_line_numbers(data, device_sections)

        # Generate document sections
        sections = []
        sections.append(self.format_header(data))
        sections.append(self.format_toc(data, line_numbers))
        sections.append(self.format_separator())

        # Add device sections
        for key in sorted(data.keys()):
            sections.append(device_sections[key])

        return '\n'.join(sections)

    def _calculate_line_numbers(self, data: Dict, device_sections: Dict[str, str]) -> Dict[str, int]:
        """
        Calculate the line number where each device section starts.

        Args:
            data: Device data dictionary
            device_sections: Pre-generated device sections

        Returns:
            Dictionary mapping device key to line number
        """
        # We need to do this in two passes because TOC includes line numbers
        # First pass: estimate line numbers
        line_numbers_estimate = {}
        current_line = 1

        # Count lines in header
        header = self.format_header(data)
        current_line += header.count('\n') + 1

        # Estimate TOC lines (header + table header + devices + footer)
        toc_estimate = 5 + len(data)
        current_line += toc_estimate

        # Count lines in separator
        separator = self.format_separator()
        current_line += separator.count('\n') + 1

        # First pass estimates
        for key in sorted(data.keys()):
            line_numbers_estimate[key] = current_line
            section_lines = device_sections[key].count('\n') + 1
            current_line += section_lines

        # Second pass: generate actual TOC and recalculate
        toc = self.format_toc(data, line_numbers_estimate)
        toc_lines = toc.count('\n') + 1

        # Recalculate with actual TOC length
        line_numbers = {}
        current_line = 1
        current_line += header.count('\n') + 1
        current_line += toc_lines
        current_line += separator.count('\n') + 1

        for key in sorted(data.keys()):
            line_numbers[key] = current_line
            section_lines = device_sections[key].count('\n') + 1
            current_line += section_lines

        return line_numbers
    
    @abstractmethod
    def format_header(self, data: Dict) -> str:
        """Format document header."""
        pass
    
    @abstractmethod
    def format_toc(self, data: Dict, line_numbers: Dict[str, int]) -> str:
        """Format table of contents."""
        pass
    
    @abstractmethod
    def format_separator(self) -> str:
        """Format separator between TOC and devices."""
        pass
    
    @abstractmethod
    def format_device_section(self, key: str, device: Dict) -> str:
        """Format a single device section."""
        pass
    
    @staticmethod
    def convert_unit(unit: str) -> str:
        """Convert unit representation (e.g., 'C -> ℃)."""
        return unit.replace("\\'C", "℃").replace("'C", "℃")
    
    @staticmethod
    def collect_labels(device: Dict) -> str:
        """Collect unique labels from device values."""
        labels = set()
        for value in device['values'].values():
            if value['label']:
                labels.add(value['label'])
        return '、'.join(sorted(labels)) if labels else '-'


class MarkdownFormatter(BaseFormatter):
    """Markdown format generator."""
    
    def format_header(self, data: Dict) -> str:
        return f"# 対応済みModbusセンサ一覧\n\n総センサ数: {len(data)}\n"
    
    def format_toc(self, data: Dict, line_numbers: Dict[str, int]) -> str:
        lines = ["## 📌 センサ一覧\n"]
        lines.append("| No. | センサ | 計測可能な項目 |")
        lines.append("|:---:|--------|---------------|")

        for idx, key in enumerate(sorted(data.keys()), start=1):
            device = data[key]
            labels = self.collect_labels(device)
            anchor = key.lower().replace('_', '-').replace('-', '')
            lines.append(f"| {idx} | [{key}](#{anchor}) | {labels} |")

        return '\n'.join(lines)
    
    def format_separator(self) -> str:
        return "\n---\n\n## 🔧 センサ詳細\n"

    def format_device_section(self, key: str, device: Dict) -> str:
        lines = [f"### {key}\n"]

        # Basic information
        lines.append(f"**センサ名:** {device['name']}  \n")
        
        # URL information
        if device.get('url'):
            lines.append("**データシート・製品情報（参考）:**  ")
            for url in device['url']:
                lines.append(f"- <{url}>  ")
            lines.append("")
        
        # Note information
        if device.get('note'):
            lines.append("**備考:**  ")
            for note in device['note']:
                lines.append(f"- {note}  ")
            lines.append("")
        
        # Values table
        lines.append("**センサー値:**\n")
        lines.append("| sub_id | 項目 | 単位 |")
        lines.append("|:------:|------|------|")
        
        for sub_id in sorted(device['values'].keys(), key=lambda x: int(x)):
            value = device['values'][sub_id]
            label = value['label'] if value['label'] else '*-*'
            unit = self.convert_unit(value['unit']) if value['unit'] else '*-*'
            unit_display = f"`{unit}`" if unit != '*-*' else unit
            lines.append(f"| {sub_id} | {label} | {unit_display} |")
        
        return '\n'.join(lines) + '\n'


class AsciiDocFormatter(BaseFormatter):
    """AsciiDoc format generator."""
    
    def format_header(self, data: Dict) -> str:
        lines = [
            "= 対応済みModbusセンサ一覧",
            ":toc: left",
            ":toclevels: 2\n",
            f"総センサ数: {len(data)}\n"
        ]
        return '\n'.join(lines)
    
    def format_toc(self, data: Dict, line_numbers: Dict[str, int]) -> str:
        lines = ["== 📌 センサ一覧\n"]
        lines.append('[cols="1,2,3", options="header"]')
        lines.append("|===")
        lines.append("| No. | センサ | 計測可能な項目\n")

        for idx, key in enumerate(sorted(data.keys()), start=1):
            device = data[key]
            labels = self.collect_labels(device)
            lines.append(f"| {idx}")
            lines.append(f"| <<{key}>>")
            lines.append(f"| {labels}\n")

        lines.append("|===\n")
        return '\n'.join(lines)
    
    def format_separator(self) -> str:
        return "\n== 🔧 センサ詳細\n"

    def format_device_section(self, key: str, device: Dict) -> str:
        lines = [f"=== {key}\n"]

        # Basic information
        lines.append(f"*センサ名:* {device['name']} +\n")
        
        # URL information
        if device.get('url'):
            lines.append("*データシート・製品情報（参考）:* +")
            for url in device['url']:
                lines.append(f"- {url} +")
            lines.append("")
        
        # Note information
        if device.get('note'):
            lines.append("*備考:* +")
            for note in device['note']:
                lines.append(f"- {note} +")
            lines.append("")
        
        # Values table
        lines.append("*センサー値:*\n")
        lines.append('[cols="1,3,2", options="header"]')
        lines.append("|===")
        lines.append("| sub_id | 項目 | 単位\n")
        
        for sub_id in sorted(device['values'].keys(), key=lambda x: int(x)):
            value = device['values'][sub_id]
            label = value['label'] if value['label'] else '_-_'
            unit = self.convert_unit(value['unit']) if value['unit'] else '_-_'
            unit_display = f"`{unit}`" if unit != '_-_' else unit
            lines.append(f"| {sub_id}")
            lines.append(f"| {label}")
            lines.append(f"| {unit_display}\n")
        
        lines.append("|===\n")
        return '\n'.join(lines)


# ============================================================================
# Main Documentation Generator
# ============================================================================

class ModbusDocGenerator:
    """Main class for generating Modbus device documentation."""
    
    def __init__(self):
        self.device_data = {}
    
    def generate(self, input_file: str) -> Dict:
        """
        Generate device documentation from source file.
        
        Args:
            input_file: Path to input C++ source file
            
        Returns:
            Dictionary containing device data
        """
        # Read input file
        try:
            with open(input_file, 'r', encoding='utf-8') as f:
                content = f.read()
            print(f"✓ Successfully read file: {input_file}")
        except Exception as e:
            print(f"✗ Error: File not found: {input_file}")
            raise
        
        # Parse device information
        print("Parsing getDeviceName()...")
        device_info = DeviceInfoExtractor.parse(content)
        print(f"✓ Found {len(device_info)} device entries")
        
        # Parse unit information
        print("Parsing getDeviceUnit()...")
        unit_info = UnitInfoExtractor.parse(content)
        
        # Count unique sensor combinations
        unit_count = sum(len(units) for units in unit_info.values())
        print(f"✓ Found unit information for {unit_count} device-sensor combinations")
        
        # Merge device and unit information
        print("Merging device data...")
        self.device_data = self._merge_data(device_info, unit_info)
        print(f"✓ Generated {len(self.device_data)} complete device entries\n")
        
        return self.device_data
    
    def _merge_data(self, device_info: Dict, unit_info: Dict) -> Dict:
        """
        Merge device and unit information.
        
        Args:
            device_info: Device information from getDeviceName
            unit_info: Unit information from getDeviceUnit (keyed by sensor_var)
            
        Returns:
            Merged device data
        """
        result = {}
        
        for key, info in device_info.items():
            sensor_var = info['sensor_var']
            
            # Build values dictionary
            values = {}
            for sub_id in info['values'].keys():
                # Find unit info for this sensor
                unit_data = None
                if sensor_var in unit_info:
                    unit_data = unit_info[sensor_var].get(sub_id)
                
                values[sub_id] = {
                    'label': unit_data['label'] if unit_data else '',
                    'unit': unit_data['unit'] if unit_data else ''
                }
            
            result[key] = {
                'name': info['name'],
                'url': info.get('url', []),
                'note': info.get('note', []),
                'values': values
            }
        
        return result
    
    def save_json(self, output_path: str, data: Dict):
        """Save data as JSON."""
        try:
            with open(output_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, ensure_ascii=False, indent=2)
            print(f"✓ Successfully saved to: {output_path}")
        except Exception as e:
            print(f"✗ Error saving JSON: {e}")
            raise
    
    def save_markdown(self, output_path: str, data: Dict):
        """Save data as Markdown."""
        try:
            formatter = MarkdownFormatter()
            content = formatter.format_document(data)
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"✓ Successfully saved Markdown to: {output_path}")
        except Exception as e:
            print(f"✗ Error saving Markdown: {e}")
            raise
    
    def save_asciidoc(self, output_path: str, data: Dict):
        """Save data as AsciiDoc."""
        try:
            formatter = AsciiDocFormatter()
            content = formatter.format_document(data)
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"✓ Successfully saved AsciiDoc to: {output_path}")
        except Exception as e:
            print(f"✗ Error saving AsciiDoc: {e}")
            raise


# ============================================================================
# Command Line Interface
# ============================================================================

def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Generate Modbus device documentation from C++ source code'
    )
    parser.add_argument(
        '-i', '--input',
        default='modbus_manager.cpp',
        help='Input C++ source file (default: modbus_manager.cpp, searches in src/modbus/)'
    )
    parser.add_argument(
        '-o', '--output',
        default='SUPPORTED_MODBUS_DEVICES',
        help='Output file base name (default: SUPPORTED_MODBUS_DEVICES in notes/)'
    )
    parser.add_argument(
        '-f', '--format',
        choices=['json', 'markdown', 'asciidoc', 'all'],
        default='markdown',
        help='Output format (default: markdown)'
    )
    
    args = parser.parse_args()
    
    print("\n=== Starting Modbus Device Documentation Generation ===\n")
    
    # Resolve input path
    try:
        input_path = PathResolver.resolve_input_path(args.input)
        print(f"✓ Resolved input file: {input_path}")
    except FileNotFoundError as e:
        print(f"✗ Error: {e}")
        return 1
    
    # Generate documentation
    generator = ModbusDocGenerator()
    try:
        data = generator.generate(input_path)
    except Exception as e:
        print(f"\n✗ Fatal error: {e}")
        return 1
    
    # Resolve output path
    output_base = PathResolver.resolve_output_path(args.output, 'SUPPORTED_MODBUS_DEVICES')
    
    # Remove extension if provided
    for ext in ['.json', '.md', '.adoc']:
        if output_base.endswith(ext):
            output_base = output_base[:-len(ext)]
            break
    
    # Save in requested format(s)
    try:
        if args.format in ['json', 'all']:
            generator.save_json(f"{output_base}.json", data)
        
        if args.format in ['markdown', 'all']:
            generator.save_markdown(f"{output_base}.md", data)
        
        if args.format in ['asciidoc', 'all']:
            generator.save_asciidoc(f"{output_base}.adoc", data)
        
        print(f"\n=== Generation Complete ===")
        print(f"Total devices documented: {len(data)}")
        return 0
        
    except Exception as e:
        print(f"\n✗ Error during save: {e}")
        return 1


if __name__ == '__main__':
    exit(main())