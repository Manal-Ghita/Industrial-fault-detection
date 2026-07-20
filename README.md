# Industrial Fault Detection — Embedded AI on STM32

[![Python](https://img.shields.io/badge/Python-3.10%2B-blue?logo=python)](https://python.org)
[![TensorFlow](https://img.shields.io/badge/TensorFlow-2.x-orange?logo=tensorflow)](https://tensorflow.org)
[![STM32](https://img.shields.io/badge/STM32-L4R9I-blue?logo=stmicroelectronics)](https://st.com)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![Status](https://img.shields.io/badge/Status-Complete-brightgreen)](https://github.com/Manal-Ghita/industrial-fault-detection-stm32)

> **End-to-end deep learning pipeline for predictive maintenance — from data analysis to real-time inference on a Cortex-M4 microcontroller.**

--- 

## Overview

This project implements a multi-class fault detection system for industrial machines, trained on real sensor data and deployed on an **STM32L4R9I-Discovery** board using **STM32CubeAI**.

The system classifies machine operating conditions into five categories in real time:

| Class | Description |
|---|---|
| No Failure | Normal operation |
| HDF | Heat Dissipation Failure |
| OSF | Overstrain Failure |
| PWF | Power Failure |
| TWF | Tool Wear Failure |

**Key challenge addressed:** The dataset is severely imbalanced (96.6% non-failure samples). A naive model achieves 98% accuracy while completely missing every real failure — a textbook accuracy paradox. This project resolves it through a combined SMOTE + RandomUnderSampler rebalancing strategy, bringing failure recall from 0.00 to 0.89–1.00 across all fault types.

**Deployment highlight:** The trained model is exported to C via STM32CubeAI and runs inference on the Cortex-M4 with 45,000 MACC, 180 KiB Flash, and 5 KiB RAM — well within the hardware constraints of the discovery board.

---

## Results

### Model Comparison

| Metric | Baseline (imbalanced) | Rebalanced model |
|---|---|---|
| Overall Accuracy | ~98% | ~86% |
| HDF Recall | 0.00 | **1.00** |
| OSF Recall | 0.06 | **0.94** |
| PWF Recall | 0.25 | **0.89** |
| TWF Recall | 0.00 | **0.89** |
| Macro F1 | 0.17 | **0.47** |

> The rebalanced model trades raw accuracy for dramatically improved fault detection — the correct priority for any industrial maintenance application where missing a real failure is more costly than a false alarm.

### On-Board Inference (STM32)

RESULTAT FINAL : 41/50 correctes
PRECISION      : 82.0%
DECISION       : Modele valide sur STM32

**82% accuracy running directly on the Cortex-M4**, consistent with the Python validation results and the known recall/precision trade-off of the SMOTE-trained model.

---

## Architecture

### Deep Learning Model

Input (6 features)
→ Dense(256, ReLU, L2) → BatchNorm → Dropout(0.3)
→ Dense(128, ReLU, L2) → BatchNorm → Dropout(0.3)
→ Dense(64,  ReLU, L2) → BatchNorm → Dropout(0.3)
→ Dense(5, Softmax)

- Loss: `categorical_crossentropy`
- Optimizer: Adam
- Epochs: 30

### Rebalancing Strategy

Applied on training set only — test set always reflects real-world distribution:

| Class | Before | After (SMOTE) |
|---|---|---|
| No Failure | 7,714 | 122 |
| HDF | 92 | 92 |
| OSF | 62 | 92 |
| PWF | 73 | 92 |
| TWF | 36 | 92 |

### Embedded Deployment

Complexity : ~45,000 MACC
Used Flash : ~180 KiB
Used RAM   :   ~5 KiB
Target     : STM32L4R9AIIx (Cortex-M4, 2MB Flash, 640KB RAM)

### PC ↔ STM32 Communication Protocol

A binary handshake over UART (115200 baud) keeps both sides synchronized:

```text
PC                            STM32
 |--- 0xAB 0x00 ----------->  |   Ready to send
 |<-- 0xCD 0x00 ------------  |   Ready to receive
 |--- 6 × float32  (24B) -->  |   Normalized sensor features
 |                            |   Cortex-M4 runs DNN inference
 |<-- 5 × uint8  (5B)  -----  |   Class scores (value × 255)
```

## Dataset

**AI4I 2020 Predictive Maintenance Dataset** — UCI Machine Learning Repository  
10,000 samples · 6 input features · 5 output classes

| Feature | Type | Description |
|---|---|---|
| Type | string (L/M/H) | Machine quality variant |
| Air temperature [K] | float | Sensor reading |
| Process temperature [K] | float | Sensor reading |
| Rotational speed [rpm] | int | Sensor reading |
| Torque [Nm] | float | Sensor reading |
| Tool wear [min] | int | Sensor reading |

> `RNF` (Random Failure) is excluded — only 1 out of 19 labeled instances aligns with `Machine failure == 1`, making it statistically inconsistent as a target.

---

## Repository Structure

```text
industrial-fault-detection-stm32/
├── notebooks/
│   └── TP_IA_EMBARQUEE_fini.ipynb    ← Full training pipeline (Google Colab)
├── data/
│   └── ai4i2020.csv                  ← AI4I 2020 dataset
├── stm32/
│   ├── ai_test_project/
│   │   ├── Core/Src/
│   │   │   ├── main.c                ← Entry point
│   │   │   ├── usart.c               ← USART2 remapped to PA2/PA3
│   │   │   └── syscalls.c            ← Minimal C stdlib stubs
│   │   └── X-CUBE-AI/App/
│   │       └── app_x-cube-ai.c       ← Inference loop + UART protocol
│   ├── predictive_model_m1.h5        ← Trained Keras model (STM32CubeAI-compatible)
│   ├── X_test_ai4i.npy               ← Normalized test features
│   ├── y_test_ai4i.npy               ← Test labels
│   └── test_ai4i.py                  ← PC-side test script
├── requirements.txt
└── README.md
```

## How to Run

### Part 1 — Training (Google Colab)

1. Open `notebooks/TP_IA_EMBARQUEE_fini.ipynb` in Google Colab
2. Mount your Drive and update the dataset path:

```python
drive.mount('/content/drive')
donnees = pd.read_csv("/content/drive/MyDrive/path/to/ai4i2020.csv")
```

3. Run all cells in order.

### Part 2 — On-Board Inference (STM32)

**Requirements:** STM32CubeIDE ≥ 1.16, X-CUBE-AI ≥ 10.2.0

1. Open the project in `stm32/ai_test_project/` with STM32CubeIDE
2. Build (`Ctrl+B`) and flash to the board (`Ctrl+F11`)
3. Install Python dependencies and run the test script:

```bash
pip install pyserial numpy
python stm32/test_ai4i.py   # adjust PORT variable to your COM port
```

---

## Dependencies

tensorflow >= 2.x
scikit-learn
imbalanced-learn
pandas
numpy
matplotlib
seaborn
pyserial

```bash
pip install -r requirements.txt
```

---

## Technical Notes

- USART2 is manually remapped to **PA2 (TX) / PA3 (RX)** to match the ST-Link VCP wiring on the 32L4R9IDISCOVERY board — the CubeIDE default assignment (PD5/PA15) targets external connector pins not connected to the ST-Link.
- `syscalls.c` is added manually as it is absent from the X-CUBE-AI generated project but required by the linker for minimal C stdlib support.
- The binary handshake protocol prevents debug UART messages from being misinterpreted as sensor data payloads.

---

## License

MIT — see [LICENSE](LICENSE) for details.
