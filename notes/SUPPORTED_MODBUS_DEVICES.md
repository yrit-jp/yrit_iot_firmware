# 対応済みModbusセンサ一覧

総センサ数: 8

## 📌 センサ一覧

| No. | センサ | 計測可能な項目 |
|:---:|--------|---------------|
| 1 | [KM_N1_FLK](#kmn1flk) | 瞬間電力、積算電力量 |
| 2 | [KW7M](#kw7m) | 瞬間電力、積算電力量 |
| 3 | [SEN0604](#sen0604) | 水素イオン濃度、温度、相対湿度、電気伝導度 |
| 4 | [SENSING_EYE_73X](#sensingeye73x) | 温度、電気伝導率・電気抵抗率 |
| 5 | [WLD-PA_R](#wldpar) | 瞬間電力、積算電力量 |
| 6 | [XY-MD02](#xy-md02) | 温度、相対湿度 |
| 7 | [YPMS_482D](#ypms482d) | 大気圧、温度、溶存酸素量、酸素濃度、酸素飽和度 |
| 8 | [YPMS_482P](#ypms482p) | 水素イオン濃度 |

---

## 🔧 センサ詳細

### KM_N1_FLK

**センサ名:** KM_N1_FLK  

**データシート・製品情報（参考）:**  
- <https://www.fa.omron.co.jp/products/family/3413/>  

**センサー値:**

| sub_id | 項目 | 単位 |
|:------:|------|------|
| 0 | 積算電力量 | `kWh` |
| 1 | 瞬間電力 | `W` |

### KW7M

**センサ名:** KW7M  

**データシート・製品情報（参考）:**  
- <https://industry.panasonic.com/jp/ja/products/fasys/eco/eco/kw7m>  

**センサー値:**

| sub_id | 項目 | 単位 |
|:------:|------|------|
| 0 | 積算電力量 | `kWh` |
| 1 | 瞬間電力 | `kW` |

### SEN0604

**センサ名:** SEN0604  

**データシート・製品情報（参考）:**  
- <https://www.switch-science.com/products/9674>  

**センサー値:**

| sub_id | 項目 | 単位 |
|:------:|------|------|
| 0 | 相対湿度 | `%RH` |
| 1 | 温度 | `℃` |
| 2 | 電気伝導度 | `uS/cm` |
| 3 | 水素イオン濃度 | `pH` |

### SENSING_EYE_73X

**センサ名:** SENSING_EYE_73X  

**データシート・製品情報（参考）:**  
- <https://techmo.co.jp/sensing_eye/products/electrical_conductivity_7732/>  
- <https://techmo.co.jp/sensing_eye/products/electrical_resistivity_7731/>  

**備考:**  
- 電気伝導率か電気抵抗率かの自動判別はできない  

**センサー値:**

| sub_id | 項目 | 単位 |
|:------:|------|------|
| 0 | 電気伝導率・電気抵抗率 | *-* |
| 1 | 温度 | `℃` |

### WLD-PA_R

**センサ名:** WLD-PA_R  

**データシート・製品情報（参考）:**  
- <https://www.watanabe-electric.co.jp/products/detail/13482/>  

**センサー値:**

| sub_id | 項目 | 単位 |
|:------:|------|------|
| 0 | 積算電力量 | `kWh` |
| 1 | 瞬間電力 | `W` |

### XY-MD02

**センサ名:** XY-MD02  

**データシート・製品情報（参考）:**  
- <https://www.idbsmart.cz/wp-content/uploads/2024/05/xy-md02-manual.pdf>  

**センサー値:**

| sub_id | 項目 | 単位 |
|:------:|------|------|
| 0 | 相対湿度 | `%RH` |
| 1 | 温度 | `℃` |

### YPMS_482D

**センサ名:** YPMS_482D  

**データシート・製品情報（参考）:**  
- <https://www.y-dkk.com/shopytd/products/detail/40>  

**センサー値:**

| sub_id | 項目 | 単位 |
|:------:|------|------|
| 0 | 溶存酸素量 | `mg/L` |
| 1 | 酸素濃度 | `%O2` |
| 2 | 酸素飽和度 | `%SAT` |
| 3 | 大気圧 | `hPa` |
| 4 | 温度 | `℃` |

### YPMS_482P

**センサ名:** YPMS_482P  

**データシート・製品情報（参考）:**  
- <https://www.y-dkk.com/shopytd/products/detail/37>  

**備考:**  
- ORP 版は未対応  

**センサー値:**

| sub_id | 項目 | 単位 |
|:------:|------|------|
| 0 | 水素イオン濃度 | `pH` |
