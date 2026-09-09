# f_Solvus_G_Test1_0_0_1000_400_0 — slowly-dissolving solid solution

Sanidine–albite solvus (Th. Wagner's parent record), 400 °C / 1000 bar.
57 dependent components, 9 independent components, 20 phases; ThermoFun-backed.

## What it is for

A **single ternary feldspar** (`Plagioclase`: Albite / Anorthite / Sanidine) with **no
interchangeable twin** — deliberately unlike `f_/j_Solvus`, whose two feldspars share an identical
end-member list and are handled by separate twin-detection machinery.

It exists to exercise a phase that **dissolves slowly**: falling monotonically over thousands of
solver iterations while never reaching zero. That signature is what `pa_MbTrendPhaseDecay` (Leal 2014
§2.3.3's two-clause unstable-phase test) is meant to detect, and no other project in the corpus
produces it.

## How to drive it

The shipped recipe does **not** show decay on its own — a cold solve starts every phase near its
floor, so phases *grow* into place. The signature needs:

1. a **warm restart** (`NEED_GEM_SOP`) from the shipped converged state, and
2. **Al, Na and K scaled together** by ~0.998. Scaling Al alone does not work: each feldspar
   end-member needs Al *and* an alkali, so Na and K simply re-form the phase. Si is irrelevant here
   (it is in large excess as quartz).

```
build-solvus/bin/decay_probe --phase=Plagioclase --ic=Al,Na,K --factor=0.998 \
    Resources/gems3k/f_Solvus_G_Test1_0_0_1000_400_0/f_Solvus_G_Test1_0_0_1000_400_0-dat.lst
```

| Al,Na,K scale | iterations | longest monotone fall |
|---|---|---|
| 0.995 | 71 | 5 |
| **0.998** | **7007** | **6958** |
| 0.9985 | 7007 | 6957 |

The stability boundary is at **0.99885** — a 0.115 % composition change flips the phase from present
to absent, so the slow-decay regime is a narrow band just inside it. Mineral appearance/disappearance
is inherently steep; that is why the driving force has to be this small to make dissolution slow.

## Status

- **Negative control — passes.** With `pa_MbTrendPhaseDecay = 50` the run is byte-identical to the
  field off: a genuine 7000-step decay does not make the detector misfire.
- **Positive case — not yet produced.** The detector sits in a failure-only retry
  (`ipm_optima.cpp`, inside `if( !result.succeeded )`), and this run *converges*, so the tier never
  executes. A cold solve at the boundary fails but grows rather than decays, so it is a no-op there
  too. A run that both decays *and* fails has not been found.
- **The more interesting use this exposed**: `factor=0.998` costs **7007** iterations where
  `factor=0.995` costs **71** — a ~100× penalty on nearly identical compositions, caused by slow
  dissolution in a run that *succeeds*. Acting on the decay there would be a cost win the detector's
  current placement forecloses.

## Notes

- Unusually rich **T,P grid: 28 temperatures (298–973 K) × 21 pressures (1000–3000 bar)** — the
  widest in the corpus, so this project is also the natural choice for any test needing a T or P
  lever.
- Optima traces are in internally rescaled units. Divide by `ScFact = pa_DG / Σb = 1000/1.7851 =
  560.18` before comparing with GEMS-GUI amounts.
- Native converges in 70 iterations, AOP in 823, both to `G = -33.22745856`.
