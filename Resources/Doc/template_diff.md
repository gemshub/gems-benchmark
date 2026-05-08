## Comparator

Comparator class used to compare two values, strings, vector-like objects (std::list, std::vector, std::set, etc) or map-like objects (std::map, std::unordered_map, etc).

Any string split by space symbols and compare as string array. If both strings contain numbers, then we compare two double values.

Compare of float-type numbers methods:

1. RelDiffMore  Indicate difference if the absolute difference of compared numbers n1 > n2 is greater than a product of n1 by the indicated "eps" value (i.e. not "essentially equal")

2. AprDiffMore  Indicate difference if the absolute difference of compared numbers n1 < n2  is greater than a product of n1 by the indicated "eps" value (i.e. not "approximately equal")

3. AbsDiffMore  Indicate difference if the absolute difference is greater than a  "eps" value (i.e. not " absolute tolerance")

4. SigDigLess   Indicate difference if the number of identical significant digits is less than given as value

5. LogDigLess   Indicate difference if the number of identical significant digits of log is less than given as value



> *KeyValue must be all different keys, no groups*


## Difference template file


template-diff.json   (comments 10.02.2020)

"doctype":      Semantic type of structured document containing data to compare.

    "GEMS3K-dbr-old"    Customary key-value format used in standalone GEMS3K code
    "GEMS3K-dbr-json"   JSON key-value format for standalone GEMS3K code as an upgrade from the old key-value format.
    ....

"key_pattern":     Regular expression  to extract key-value     ( "<([^>]*)>")

"comment_pattern": Regular expression  to remove comments  ("#([^\\\n]*)\\\n")


"eps_number"    Description float-type values comparison should be done ( numeric values not from "datafields" )


"datafields":   List/map of data keys (keywords) for which advanced comparison should be done.

                Rules:  (1) the order of elements in his list is arbitrary;
                        (2) if a data key is absent from this list, then a plain diff (equal value(s) as strings or not) is performed;
                        (3) The order of data fields in the document is arbitrary (but in array fields the order of values is fixed).

Element of "datafields" list:

    "datakey": field_key (for example, "<IS>")  Data field name in customary key-values format

    "comment": comment about the physical meaning of the data (not processed by code)

    "compare": compare_type (for example, "sig_dig_less" to compare significant decimal digits and sign of normalized float-type numbers)

    "eps": what is considered as different (for example, 5 to mark as different two numbers with less than 5 significant digits)  or  tolerance in a sense of relative difference ("essentially equal")

                Rules:  (4) If the field contains several values, the same comparison operation applies to each value.
                        (5) For comparison failed for a value with a given index, the values are indicated with the index.
                        (6) If the number of values is different for this field in compared files, the comparison fails
                            and the numbers of values is reported for both files.

compare_types:

  "sig_dig_less"  indicate difference if the number of identical significant digits is less than given as value

  "log_dig_less"  indicate difference log of values if the number of identical significant digits is less than given as value

  "abs_def_more" indicate difference if the value is greater than indicated "diff" value

  "rel_def_more" indicate difference if the absolute difference of compared numbers n1 > n2 is greater than a product of n1 by the indicated "eps" value (i.e. not "essentially equal")
  
  "apr_def_more" indicate difference if the absolute difference of compared numbers n1 < n2 is greater than a product of n1 by the indicated "eps" value (i.e. not "approximately equal")

