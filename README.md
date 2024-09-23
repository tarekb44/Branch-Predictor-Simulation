
# Branch Predictor Simulator

## Overview

This project simulates and measures the accuracy of various branch predictors:
- **Always Taken/Not Taken**
- **Bimodal (Single-Bit and Two-Bit History)**
- **Gshare**
- **Tournament Predictor**
- **Branch Target Buffer (BTB) using a single-bit bimodal predictor**

The simulator reads tracefiles and evaluates the accuracy of each branch prediction algorithm. Predictors run on their own threads, ensuring efficient and fast simulation.

## How It Works

The branch predictor simulation reads a tracefile in the following format:
```
0x7f4072aa223f NT 0x7f4072aa2280
0x7f4072aa224c NT 0x7f4072aa2280
0x7f4072aa225d T 0x7f4072aa2266
```
- `0x7f4072aa223f`: Address of the branch instruction
- `NT/T`: Actual branch outcome (`NT` for not taken, `T` for taken)
- `0x7f4072aa2280`: Target address

The predictors utilize this data to simulate their behavior and measure prediction accuracy.

## Features

### Predictors Included
- **Always Taken/Not Taken:** Simplest predictors, always assume branches are taken or not taken.
- **Bimodal Predictor (Single-Bit and Two-Bit):** Uses a table to maintain predictions for each address.
- **Gshare:** Combines global branch history with branch address to make predictions.
- **Tournament Predictor:** Uses both gshare and bimodal predictors and a selector to choose the most accurate.
- **Branch Target Buffer (BTB):** Predicts the target address of branches using a bimodal predictor.

## How to Use

### Compile
Compile the code using `gcc` or any C compiler with pthread support:
```bash
gcc -pthread -o branch_predictor branch_predictor.c
```

### Run
Execute the compiled program with the tracefile as input:
```bash
./branch_predictor input_trace.txt output.txt
```

- `input_trace.txt`: Tracefile with the branch data.
- `output.txt`: File where the prediction results will be saved.

### Tracefile
Ensure your tracefile has UNIX line endings.

## Example Outputs

Below are example outputs from the simulator, demonstrating the accuracy of each prediction algorithm.

### Always Taken/Not Taken Predictors
```
Always Taken Predictor:
Correctly Predicted: 1500000 / 2500000

Always Not Taken Predictor:
Correctly Predicted: 800000 / 2500000
```

### Bimodal Predictor (Single-Bit History)
```
Bimodal Predictor (Single-Bit History):
Table Size 16: Correctly Predicted: 700000 / 2500000
Table Size 32: Correctly Predicted: 720000 / 2500000
Table Size 128: Correctly Predicted: 800000 / 2500000
Table Size 256: Correctly Predicted: 850000 / 2500000
Table Size 512: Correctly Predicted: 870000 / 2500000
Table Size 1024: Correctly Predicted: 900000 / 2500000
Table Size 2048: Correctly Predicted: 950000 / 2500000
```

### Bimodal Predictor (Two-Bit History)
```
Bimodal Predictor (Two-Bit History):
Table Size 16: Correctly Predicted: 750000 / 2500000
Table Size 32: Correctly Predicted: 770000 / 2500000
Table Size 128: Correctly Predicted: 820000 / 2500000
Table Size 256: Correctly Predicted: 870000 / 2500000
Table Size 512: Correctly Predicted: 910000 / 2500000
Table Size 1024: Correctly Predicted: 940000 / 2500000
Table Size 2048: Correctly Predicted: 960000 / 2500000
```

### Gshare Predictor
```
Gshare Predictor:
GHR Size 3: Correctly Predicted: 780000 / 2500000
GHR Size 4: Correctly Predicted: 800000 / 2500000
GHR Size 5: Correctly Predicted: 820000 / 2500000
GHR Size 6: Correctly Predicted: 840000 / 2500000
GHR Size 7: Correctly Predicted: 860000 / 2500000
GHR Size 8: Correctly Predicted: 880000 / 2500000
GHR Size 9: Correctly Predicted: 900000 / 2500000
GHR Size 10: Correctly Predicted: 920000 / 2500000
GHR Size 11: Correctly Predicted: 940000 / 2500000
```

### Tournament Predictor
```
Tournament Predictor:
Correctly Predicted: 980000 / 2500000
```

### Branch Target Buffer (BTB) Simulation
```
Branch Target Buffer (BTB) Predictor:
Correctly Predicted: 600000 / 1200000
Attempted Predictions: 1200000 / 2500000
```
