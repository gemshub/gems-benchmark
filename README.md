# gems-benchmark
Run benchmark and speed tests calculations for [GEMS3K](https://github.com/gemshub/GEMS3K.git)

A group of test applications is used to verify that the same results are obtained and to assess any improvements in speed, number of iterations, calculations, and convergence.


The tests run over several exported test projects (`Resources/gems3k`).


### recalc_all

*recalc_all* the CLI utility to recalculate all GEMS3K test projects and export the result GEMS3K I/O files to compare.

```sh
> ./recalc_all gems3k gems3k-v4.5.5
```

### collect_metrics

*collect_metrics* the CLI utility to get iteration, convergence, and performance metrics;  speed tests by changing the composition (random), also compare cold vs warm start.

```sh
> ./collect_metrics gems3k-v4.5.5
```


### compare_dirs

*compare_dirs*  the command analyzes two key-value files or json files or dirs with such files and prints the difference.

1. The order of data fields in the document is arbitrary (but in array fields the order of values is fixed)

2. Comparing floating point numbers, choosing epsilon depends on the context, and determines how equal you want the numbers to be.

3. Any string split by space symbols and compare as a string array. If both strings contain numbers, then we compare two double values.

[Difference template file](Resources/Doc/template_diff.md) can be used to set up a custom benchmark.

```sh
> ./compare_dirs -d -r -t ".*-dbr-[\d-]*.*" -rd  dbr_diff.json -a  0.1e-6 -j gems3k -j gems3k-v4.5.5
```

Parameters

```sh
Usage: compare_dirs [ option(s) ] -k|-j PATH_TEMPLATE -k|-j PATH_SOURCE
Compare FILES block by block
Options:
        -h,	--help			show this help message
        -f,	--files 		compare two files (default)
        -d,	--directories 		compare all files in directories
        -r,	--recursive   		recursively compare any subdirectories found
        -t,	--file-pattern PAT  	only files that match PAT

        -k,	--key-value PATH 	compare key-value file 
        -j,	--json      PATH 	compare json  file
        -rd,	--rules-document JSONF  structured document describing data to compare

        -b,	--difference   		output all differences between template and source (default)
        -i,	--consist-in   		output all data from the template not consist in the source

        -a,	--approximately EPS	approximately equal for floating types
        -e,	--essentially EPS	essentially equal for floating types
        -at,	--absolute-tolerance EPS	absolute tolerance equal for floating types(default)
        -sd,	--significant-digits EPS	the number of identical significant equal for floating types
        -lg,	--log-of-values EPS	the number of identical significant digits of log of values equal for floating types

```

### param_sensitivity_test

*param_sensitivity_test* the CLI utility to catch a specific regression: a solution-phase
interaction parameter change that produces no measurable change in the equilibrium result
(e.g. a trace species pinned at the numerical floor). Runs the Ti-in-Quartz
`TiQ_PRSV_G_MySystem` project twice with different QtzRu Margules interaction parameters and
asserts both runs converge and produce a measurably different mole fraction for the Rutile
end-member in the QtzRu solid solution. Prints per-run wall-clock time, GEMS3K's own
iteration counts, and (when GEMS3K was built with `-DENABLE_BENCHMARK_DIAGNOSTICS=ON`) the
condition-number/solve-time diagnostics; on any non-convergence it prints GEMS3K's own error
code/message instead of silently comparing garbage.

```sh
> ./param_sensitivity_test gems3k/j_TiQ_PRSV_G_MySystem_0_0_10000_900_0/j_TiQ_PRSV_G_MySystem_0_0_10000_900_0-dat.lst
```







