modC[J] =: cLgFe;
bi_[{Fe}] =: 10^cLgFe;
bi_[{Cl}] =: 1e-14/3.16227766016838 + 0.001;
bi_[{Na}] =: 3.16227766016838 + 0.001;
bi_[{H}] =: 2*55.50991531897225 + 3*(10^cLgFe) + 3.16227766016838 + 1e-14/3.16227766016838;
bi_[{O}] =: (55.50991531897225 + 3*(10^cLgFe) + 3.16227766016838) + (55.50991531897225 + 3*(10^cLgFe) + 3.16227766016838)*0.000002/55.5;
bi_[{Zz}] =: 0;

from -13
to -2
step 0.05

---

## 2026-09-05b — the exported `-dbr` did NOT match this recipe; NaOH added so that it does

**Found**: this fixture's `-dbr`, `-dch` and `-ipm` were **byte-identical to `FeNaCl_FyGt_Precip`'s**
- the same problem under two names, so despite the name it was not a high-pH variant at all. Its
shipped composition contained only **0.0027426 m NaOH** (derivable two independent ways: `Na - Cl`,
and `-(H - 2*O) - 3*Fe` assuming Fe enters as Fe(OH)3, which agree), against the
`3.16227766016838` this recipe fixes. `_Precip`'s own export does not match its recipe either
(it fixes `Na = 1.001`, i.e. 1.0 m NaOH), so **the two `info.md` files describe the SCAN each
failure was found in, not the point that got exported.**

**Done**: added `NaOH = 3.16227766016838` (= 10^0.5, the value fixed above) to the shipped
composition - `Na`, `O` and `H` each `+= 3.16227766016838`, charge untouched since NaOH is neutral.
Everything else (`Cl`, `Fe`, `Zz`, T, P, and the whole `-dch`/`-ipm`) is unchanged, so the edit is
exactly "the same system, plus the NaOH this recipe calls for".

| | before | after |
|---|---|---|
| `bIC[Na]` | 3.74261800e-03 | 3.16602028e+00 |
| `bIC[O]` | 5.55133827e+01 | 5.86756604e+01 |
| `bIC[H]` | 1.11022573e+02 | 1.14184851e+02 |
| pH | 11.4101 | **14.2876** (recipe claims 14.5) |
| native | converges, 76-252 it | **FAILS** (status 4, 25000 it = its cap) |
| AOP | converges, 53-66 it | converges, **43-62 it** |
| HOP | - | converges, 43-68 it |

So the fixture now reproduces the behaviour its own description claims ("near-total collapse")
**and** becomes a case where native collapses and Optima solves it - which is what `gems3k-fail`
exists to collect. Verified by first applying the same addition at RUNTIME
(`iteration_jitter --add Na:3.162...,O:3.162...,H:3.162...`) and checking the edited file reproduces
it exactly.

**Note on the amount.** The owner's instruction was "add 0.5m NaOH"; `10^0.5 = 3.16227766` is what
this recipe fixes and what reproduces the documented pH ~14.5, so that reading was taken. A literal
**0.5 m** was also measured and is a valid but much milder variant: **pH 13.5148**, native converges
in 286-335 iterations, AOP in 50-69. To switch, subtract 2.66227766016838 from `Na`, `O` and `H`.

**The `xDC` speciation left in this `-dbr` is now stale** (it is the old composition's answer). That
is irrelevant to a cold solve, which is how this fixture is used, and was confirmed by the
runtime-vs-file agreement above.
