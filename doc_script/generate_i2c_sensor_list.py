"""
I2C Device Documentation Generator

Extracts device metadata from i2c_manager.cpp and generates documentation
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
                project_root / 'src' / 'i2c' / input_path,
                project_root / 'src' / 'i2c' / 'i2c_manager.cpp',
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
            f"  - src/i2c/ directory"
        )
    
    @staticmethod
    def resolve_output_path(output_path: str, default_name: str) -> str:
        """
        Resolve output file path. If default name is used, output to project_root/notes/.
        
        Args:
            output_path: Output path specified by user
            default_name: Default output name (e.g., 'SUPPORTED_I2C_DEVICES')
            
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
    
    @staticmethod
    def extract_switch_body(text: str, switch_var: str) -> Optional[str]:
        """
        Extract the body of a switch statement.
        
        Args:
            text: Source code text
            switch_var: Variable name in switch statement
            
        Returns:
            Switch body text, or None if not found
        """
        pattern = rf'switch\s*\(\s*{switch_var}\s*\)\s*\{{'
        match = re.search(pattern, text)
        if not match:
            return None
        
        switch_start = match.end()
        switch_end = CppParser.find_matching_brace(text, switch_start - 1)
        if switch_end == -1:
            return None
        
        return text[switch_start:switch_end]


class CommentExtractor:
    """Extract special comments from C++ code."""
    
    PATTERNS = {
        'key': r'//\s*@\s*key:\s*(.+?)(?:\r?\n|$)',
        'name': r'//\s*@\s*name:\s*(.+?)(?:\r?\n|$)',
        'url': r'//\s*@\s*url:\s*(.+?)(?:\r?\n|$)',
        'label': r'//\s*@\s*label:\s*(.+?)(?:\r?\n|$)',
        'note': r'//\s*@\s*note:\s*(.+?)(?:\r?\n|$)'
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


class CaseBlockExtractor:
    """Extract case blocks from switch statements."""
    
    @staticmethod
    def extract_case_body(switch_body: str, case_start: int) -> str:
        """
        Extract case body using brace depth tracking to handle nested blocks.
        
        Args:
            switch_body: Body of switch statement
            case_start: Starting position of case block
            
        Returns:
            Case body text including break statement
        """
        brace_depth = 0
        i = case_start
        
        while i < len(switch_body):
            # At top level (brace_depth == 0), check for terminators
            if brace_depth == 0:
                # Check for 'break;'
                if switch_body[i:i+6] == 'break;':
                    return switch_body[case_start:i+6]
                # Check for next 'case'
                if re.match(r'\s*case\s+', switch_body[i:]):
                    return switch_body[case_start:i]
                # Check for 'default:'
                if re.match(r'\s*default\s*:', switch_body[i:]):
                    return switch_body[case_start:i]
            
            # Track brace depth
            if switch_body[i] == '{':
                brace_depth += 1
            elif switch_body[i] == '}':
                brace_depth -= 1
                if brace_depth < 0:
                    return switch_body[case_start:i]
            
            i += 1
        
        return switch_body[case_start:]
    
    @classmethod
    def extract_all_cases(cls, switch_body: str) -> Dict[str, str]:
        """
        Extract all case blocks from a switch statement, handling fall-through cases.

        Args:
            switch_body: Body of switch statement

        Returns:
            Dictionary mapping case addresses to case bodies
        """
        cases = {}
        pos = 0
        fallthrough_addresses = []

        while pos < len(switch_body):
            case_match = re.search(r'\bcase\s+(0x[0-9a-fA-F]+)\s*:', switch_body[pos:])
            if not case_match:
                break

            # Keep '0x' prefix lowercase, but make hex digits uppercase
            address_raw = case_match.group(1)
            if address_raw.startswith('0x') or address_raw.startswith('0X'):
                address = '0x' + address_raw[2:].upper()
            else:
                address = address_raw.upper()

            case_start = pos + case_match.end()
            case_body = cls.extract_case_body(switch_body, case_start)

            # Check if this is a fall-through case (empty or only whitespace/comments before next case)
            is_fallthrough = cls._is_fallthrough_case(case_body)

            if is_fallthrough:
                # Collect fall-through address
                fallthrough_addresses.append(address)
            else:
                # This case has actual code - assign it to all collected fall-through addresses
                all_addresses = fallthrough_addresses + [address]
                for addr in all_addresses:
                    cases[addr] = case_body
                fallthrough_addresses = []

            pos = case_start + len(case_body)

        return cases

    @staticmethod
    def _is_fallthrough_case(case_body: str) -> bool:
        """
        Check if a case body is a fall-through (no actual code, just falls to next case).

        Args:
            case_body: Body of a case block

        Returns:
            True if this is a fall-through case
        """
        # Remove comments and whitespace
        cleaned = re.sub(r'//.*?$', '', case_body, flags=re.MULTILINE)
        cleaned = re.sub(r'/\*.*?\*/', '', cleaned, flags=re.DOTALL)
        cleaned = cleaned.strip()

        # If nothing left, or only break/return, it's likely a fall-through
        # Fall-through cases typically have no code before the next case
        return len(cleaned) == 0 or cleaned == 'break;'


# ============================================================================
# Device Information Extraction
# ============================================================================

class DeviceInfoExtractor:
    """Extract device information from getDeviceName function."""

    @staticmethod
    def extract_if_blocks(case_body: str, recursive: bool = True) -> List[tuple]:
        """
        Extract all if blocks for different sensors in a case.

        Args:
            case_body: Body of a case block
            recursive: If True, also extract nested if blocks

        Returns:
            List of (sensor_variable, if_body) tuples
        """
        if_blocks = []
        pos = 0

        while pos < len(case_body):
            if_match = re.search(r'if\s*\(i2c_sensors\[.*?\]\.(\w+)\)\s*\{', case_body[pos:])
            if not if_match:
                break

            sensor_var = if_match.group(1)
            open_brace_pos = pos + if_match.end() - 1
            close_brace_pos = CppParser.find_matching_brace(case_body, open_brace_pos)

            if close_brace_pos == -1:
                pos += if_match.end()
                continue

            if_body = case_body[open_brace_pos + 1:close_brace_pos]
            if_blocks.append((sensor_var, if_body))

            # Recursively extract nested if blocks
            if recursive:
                nested_blocks = DeviceInfoExtractor.extract_if_blocks(if_body, recursive=True)
                if_blocks.extend(nested_blocks)

            pos = close_brace_pos + 1

        return if_blocks
    
    @staticmethod
    def extract_device_keys(if_body: str) -> List[str]:
        """
        Extract device keys from if body, excluding keys in nested if blocks.

        Args:
            if_body: Body of an if block

        Returns:
            List of device key names
        """
        # Remove nested if blocks to avoid extracting their keys
        cleaned_body = DeviceInfoExtractor._remove_nested_if_blocks(if_body)

        # Extract comments from cleaned body only
        comments = CommentExtractor.extract(cleaned_body)

        # Priority: special comment keys > default return value
        if comments.get('key'):
            return comments['key']

        # Look for default return in switch block
        default_pattern = r'default\s*:\s*return\s*"([^"]+)"'
        default_match = re.search(default_pattern, cleaned_body)
        if default_match:
            return [default_match.group(1)]

        # Look for direct return statement (for devices without switch block)
        # Note: Exclude bridge devices (DS248X, DS2485) which are not actual sensors
        direct_return_pattern = r'return\s*"([^"(]+)"'
        direct_returns = re.findall(direct_return_pattern, cleaned_body)
        if direct_returns:
            # Filter out bridge devices and empty strings
            bridge_devices = ['DS248X', 'DS2485']
            unique_names = list(set(direct_returns))
            unique_names = [name for name in unique_names
                          if name and len(name) > 1
                          and not any(name.startswith(bridge) for bridge in bridge_devices)]
            if unique_names:
                # Return the shortest name (likely the base device name)
                return [min(unique_names, key=len)]

        return []

    @staticmethod
    def _remove_nested_if_blocks(if_body: str) -> str:
        """
        Remove nested if(i2c_sensors[...]) blocks from if_body.

        Args:
            if_body: Body of an if block

        Returns:
            if_body with nested if blocks removed
        """
        result = if_body
        pos = 0

        while pos < len(result):
            nested_match = re.search(r'if\s*\(i2c_sensors\[.*?\]\.(\w+)\)\s*\{', result[pos:])
            if not nested_match:
                break

            match_start = pos + nested_match.start()
            open_brace_pos = pos + nested_match.end() - 1
            close_brace_pos = CppParser.find_matching_brace(result, open_brace_pos)

            if close_brace_pos == -1:
                pos += nested_match.end()
                continue

            # Replace the entire nested if block with a placeholder comment
            result = result[:match_start] + "/* nested if block removed */" + result[close_brace_pos + 1:]
            pos = match_start + 30  # length of placeholder comment

        return result
    
    @staticmethod
    def extract_sub_ids(if_body: str) -> List[str]:
        """
        Extract sub_id case values from if body, excluding nested if blocks.

        Args:
            if_body: Body of an if block

        Returns:
            List of sub_id values
        """
        # Remove nested if blocks to avoid extracting their case values
        cleaned_body = DeviceInfoExtractor._remove_nested_if_blocks(if_body)

        sub_ids = []
        for match in re.finditer(r'case\s+(\d+)\s*:', cleaned_body):
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
            content, r'String\s+I2C_Manager::getDeviceName\s*\([^)]+\)'
        )
        if not func_body:
            print("✗ Warning: getDeviceName function not found")
            return device_info
        
        # Extract switch body
        switch_body = CppParser.extract_switch_body(func_body, '_address')
        if not switch_body:
            print("✗ Warning: switch(_address) not found in getDeviceName")
            return device_info
        
        # Extract all cases
        cases = CaseBlockExtractor.extract_all_cases(switch_body)
        
        # Process each case
        for address, case_body in cases.items():
            # Check if case has actual code
            if not re.search(r'\bif\s*\(|return\s+', case_body):
                continue
            
            # Extract if blocks
            if_blocks = cls.extract_if_blocks(case_body)
            
            # Process each if block
            for sensor_var, if_body in if_blocks:
                # Extract device keys
                device_keys = cls.extract_device_keys(if_body)
                if not device_keys:
                    continue
                
                # Extract metadata
                comments = CommentExtractor.extract(if_body)
                sub_ids = cls.extract_sub_ids(if_body)
                
                # Store device information
                for key in device_keys:
                    if key not in device_info:
                        device_info[key] = {
                            'name': comments.get('name', [key])[0] if comments.get('name') else key,
                            'url': comments.get('url', []),
                            'note': comments.get('note', []),
                            'addresses': [],
                            'sensor_var': sensor_var,  # Add sensor_var
                            'values': {}
                        }
                    
                    # Add address if not present
                    if address not in device_info[key]['addresses']:
                        device_info[key]['addresses'].append(address)
                    
                    # Initialize sub_ids
                    for sub_id in sub_ids:
                        if sub_id not in device_info[key]['values']:
                            device_info[key]['values'][sub_id] = {'label': '', 'unit': ''}
        
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
            Dictionary mapping (address, sensor_var) to unit information
        """
        unit_info = {}
        
        # Extract function body
        func_body = CppParser.extract_function_body(
            content, r'String\s+I2C_Manager::getDeviceUnit\s*\([^)]+\)'
        )
        if not func_body:
            print("✗ Warning: getDeviceUnit function not found")
            return unit_info
        
        # Extract switch body
        switch_body = CppParser.extract_switch_body(func_body, '_address')
        if not switch_body:
            print("✗ Warning: switch(_address) not found in getDeviceUnit")
            return unit_info
        
        # Extract all cases
        cases = CaseBlockExtractor.extract_all_cases(switch_body)
        
        # Process each case
        for address, case_body in cases.items():
            # Extract if blocks
            if_blocks = DeviceInfoExtractor.extract_if_blocks(case_body)
            
            # Process each if block
            for sensor_var, if_body in if_blocks:
                # Check for switch(sub_id) or switch(value_id) block
                sub_switch_match = re.search(r'switch\s*\(\s*(sub_id|value_id)\s*\)\s*\{', if_body)

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
                        
                        # Store unit information using (address, sensor_var) as key
                        key = (address, sensor_var)
                        if key not in unit_info:
                            unit_info[key] = {}
                        
                        unit_info[key][sub_id] = {
                            'label': comments.get('label', [''])[0] if comments.get('label') else '',
                            'unit': unit,
                            'note': comments.get('note', [])
                        }
                else:
                    # Direct return without switch(sub_id) block
                    comments = CommentExtractor.extract(if_body)
                    label = comments.get('label', [''])[0] if comments.get('label') else ''
                    
                    return_match = re.search(r'return\s+"([^"]*)"', if_body)
                    unit = return_match.group(1) if return_match else ''
                    
                    # Store for '_default' key
                    key = (address, sensor_var)
                    if key not in unit_info:
                        unit_info[key] = {}
                    
                    unit_info[key]['_default'] = {
                        'label': label,
                        'unit': unit,
                        'note': comments.get('note', [])
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
    
    @staticmethod
    def format_note_summary(notes: List[str]) -> str:
        """Format note summary for TOC."""
        if not notes:
            return '-'
        note = notes[0]
        if len(notes) > 1:
            note += f" (他{len(notes)-1}件)"
        return note


class MarkdownFormatter(BaseFormatter):
    """Markdown format generator."""
    
    def format_header(self, data: Dict) -> str:
        return f"# 対応済みI2Cセンサ一覧\n\n総センサ数: {len(data)}\n"
    
    def format_toc(self, data: Dict, line_numbers: Dict[str, int]) -> str:
        lines = ["## 📌 センサ一覧\n"]
        lines.append("| No. | デバイス | I2Cアドレス | 計測可能な項目 | 備考 |")
        lines.append("|:---:|----------|------------|---------------|------|")

        for idx, key in enumerate(sorted(data.keys()), start=1):
            device = data[key]
            addresses = ', '.join(device['addresses'])
            labels = self.collect_labels(device)
            note = self.format_note_summary(device['note'])
            anchor = key.lower().replace('_', '-')
            lines.append(f"| {idx} | [{key}](#{anchor}) | `{addresses}` | {labels} | {note} |")

        return '\n'.join(lines)
    
    def format_separator(self) -> str:
        return "\n---\n\n## 🔧 センサ詳細\n"
    
    def format_device_section(self, key: str, device: Dict) -> str:
        lines = [f"### {key}\n"]

        # Basic information
        lines.append(f"**センサ名:** {device['name']}  ")
        lines.append(f"**I2Cアドレス:** `{', '.join(device['addresses'])}`  \n")
        
        # URLs
        if device['url']:
            lines.append("**データシート・製品情報（参考）:**  ")
            for url in device['url']:
                lines.append(f"- <{url}>  ")
            lines.append("")
        
        # Notes
        if device['note']:
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
            "= 対応済みI2Cセンサ一覧",
            ":toc: left",
            ":toclevels: 2\n",
            f"総センサ数: {len(data)}\n"
        ]
        return '\n'.join(lines)
    
    def format_toc(self, data: Dict, line_numbers: Dict[str, int]) -> str:
        lines = ["== 📌 センサ一覧\n"]
        lines.append('[cols="1,2,2,3,4", options="header"]')
        lines.append("|===")
        lines.append("| No. | デバイス | I2Cアドレス | 計測可能な項目 | 備考\n")

        for idx, key in enumerate(sorted(data.keys()), start=1):
            device = data[key]
            addresses = ', '.join(device['addresses'])
            labels = self.collect_labels(device)
            note = self.format_note_summary(device['note'])
            lines.append(f"| {idx}")
            lines.append(f"| <<{key}>>")
            lines.append(f"| `{addresses}`")
            lines.append(f"| {labels}")
            lines.append(f"| {note}\n")

        lines.append("|===\n")
        return '\n'.join(lines)
    
    def format_separator(self) -> str:
        return "\n== 🔧 センサ詳細\n"
    
    def format_device_section(self, key: str, device: Dict) -> str:
        lines = [f"=== {key}\n"]

        # Basic information
        lines.append(f"*センサ名:* {device['name']} +")
        lines.append(f"*I2Cアドレス:* `{', '.join(device['addresses'])}` +\n")
        
        # URLs
        if device['url']:
            lines.append("*データシート・製品情報（参考）:*\n")
            for url in device['url']:
                lines.append(f"* {url}")
            lines.append("")
        
        # Notes
        if device['note']:
            lines.append("*備考:*\n")
            for note in device['note']:
                lines.append(f"* {note}")
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

class I2CDocGenerator:
    """Main class for generating I2C device documentation."""
    
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
        
        # Count unique device-address combinations
        unit_count = sum(len(addrs) for addrs in unit_info.values())
        print(f"✓ Found unit information for {unit_count} device-address combinations")
        
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
            unit_info: Unit information from getDeviceUnit (keyed by (address, sensor_var))
            
        Returns:
            Merged device data
        """
        result = {}
        
        for key, info in device_info.items():
            sensor_var = info['sensor_var']
            addresses = info['addresses']
            
            # Collect notes from both getDeviceName and getDeviceUnit
            all_notes = list(info['note'])  # Start with notes from getDeviceName
            
            # Build values dictionary
            values = {}
            for sub_id in info['values'].keys():
                # Find unit info for this address and sensor
                unit_data = None
                for addr in addresses:
                    lookup_key = (addr, sensor_var)
                    if lookup_key in unit_info:
                        unit_data = unit_info[lookup_key].get(
                            sub_id, unit_info[lookup_key].get('_default')
                        )
                        if unit_data:
                            # Add notes from getDeviceUnit
                            if 'note' in unit_data:
                                for note in unit_data['note']:
                                    if note and note not in all_notes:
                                        all_notes.append(note)
                            break
                
                values[sub_id] = {
                    'label': unit_data['label'] if unit_data else '',
                    'unit': unit_data['unit'] if unit_data else ''
                }
            
            result[key] = {
                'name': info['name'],
                'url': info['url'],
                'note': all_notes,
                'addresses': addresses,
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
        description='Generate I2C device documentation from C++ source code'
    )
    parser.add_argument(
        '-i', '--input',
        default='i2c_manager.cpp',
        help='Input C++ source file (default: i2c_manager.cpp, searches in src/i2c/)'
    )
    parser.add_argument(
        '-o', '--output',
        default='SUPPORTED_I2C_DEVICES',
        help='Output file base name (default: SUPPORTED_I2C_DEVICES in notes/)'
    )
    parser.add_argument(
        '-f', '--format',
        choices=['json', 'markdown', 'asciidoc', 'all'],
        default='markdown',
        help='Output format (default: markdown)'
    )
    
    args = parser.parse_args()
    
    print("\n=== Starting I2C Device Documentation Generation ===\n")
    
    # Resolve input path
    try:
        input_path = PathResolver.resolve_input_path(args.input)
        print(f"✓ Resolved input file: {input_path}")
    except FileNotFoundError as e:
        print(f"✗ Error: {e}")
        return 1
    
    # Generate documentation
    generator = I2CDocGenerator()
    try:
        data = generator.generate(input_path)
    except Exception as e:
        print(f"\n✗ Fatal error: {e}")
        return 1
    
    # Resolve output path
    output_base = PathResolver.resolve_output_path(args.output, 'SUPPORTED_I2C_DEVICES')
    
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