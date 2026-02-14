# Formal Verification for Narwhal++

This directory contains formal specifications and verification artifacts for the Narwhal++ consensus protocols.

## 📁 Structure

```
formal/
├── tla/                    # TLA+ specifications
│   ├── Mysticeti.tla      # Mysticeti consensus model
│   ├── Mysticeti.cfg      # TLC model checker configuration
│   └── README.md          # TLA+ usage guide
└── README.md              # This file
```

## 🎯 Verification Goals

### Safety Properties
1. **Agreement**: No two honest nodes commit conflicting blocks
2. **Validity**: Only proposed blocks can be committed
3. **Integrity**: A block is committed at most once

### Liveness Properties
1. **Eventual Commitment**: If a leader receives 2f+1 votes, it will eventually commit
2. **Progress**: The system makes progress under honest majority

## 🔧 Tools

### TLA+ (Temporal Logic of Actions)
TLA+ is used to specify and model-check the consensus protocols.

**Installation:**
```bash
# Download TLA+ Toolbox from:
# https://github.com/tlaplus/tlaplus/releases
```

**Running the model checker:**
```bash
cd formal/tla
java -jar tla2tools.jar -config Mysticeti.cfg Mysticeti.tla
```

**What it checks:**
- Type correctness (TypeOK invariant)
- Safety properties (no conflicting commits)
- Liveness properties (eventual progress)
- Deadlock freedom

### Expected Output
```
TLC2 Version X.XX
Running in Model-Checking mode.
Checking 3 temporal properties...
Model checking completed. No error has been found.
  States analyzed: 12,456
  Distinct states: 3,421
  State queue size: 0
```

## 📊 Current Status

| Protocol | Spec Complete | Model Checked | Properties Verified |
|----------|---------------|---------------|---------------------|
| Mysticeti | ✅ Skeleton | ⏳ Pending | Safety, Liveness |
| Shoal++ | ⏳ TODO | ⏳ Pending | - |
| Tusk | ⏳ TODO | ⏳ Pending | - |

## 🚀 Next Steps

1. **Complete Mysticeti spec**: Add full DAG construction and vote aggregation
2. **Run TLC model checker**: Verify safety and liveness for small configurations (N=4, F=1)
3. **Add Shoal++ spec**: Model reputation-based anchor selection
4. **Add Tusk spec**: Model the original 2-round commit protocol
5. **Refinement mapping**: Prove that C++ implementation refines the TLA+ spec

## 📚 Resources

- [TLA+ Homepage](https://lamport.azurewebsites.net/tla/tla.html)
- [Learn TLA+](https://learntla.com/)
- [TLA+ Examples](https://github.com/tlaplus/Examples)
- [Mysticeti Paper](https://arxiv.org/abs/2310.14821)

## ⚠️ Limitations

The current specifications are **simplified models** that:
- Assume perfect message delivery (no network failures)
- Use bounded state spaces (MaxRound parameter)
- Abstract away cryptographic details

For production deployment, these assumptions must be relaxed and verified under more realistic conditions.
