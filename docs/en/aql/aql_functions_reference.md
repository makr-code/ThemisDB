# AQL Functions Reference

**Generated:** 2026-02-23  
**Source:** Auto-generated from C++ header files in `include/query/functions/`  
**Do not edit manually** – re-run `scripts/generate_aql_docs.py` to update.

---

## Table of Contents

- [String Functions](#string-functions)
- [Math Functions](#math-functions)
- [Array Functions](#array-functions)
- [Date Functions](#date-functions)
- [Document Functions](#document-functions)
- [JSON Functions](#json-functions)
- [Geo Functions](#geo-functions)
- [Vector Functions](#vector-functions)
- [Graph Functions](#graph-functions)
- [Relational Functions](#relational-functions)
- [File Functions](#file-functions)
- [Collection Functions](#collection-functions)
- [Ethics Functions](#ethics-functions)
- [Logical Functions](#logical-functions)
- [Process Functions](#process-functions)
- [ProcessMining Functions](#processmining-functions)
- [Retention Functions](#retention-functions)
- [Scheduling Functions](#scheduling-functions)
- [Statistics Functions](#statistics-functions)
- [Type Functions](#type-functions)

---

## String Functions

| Function | Description |
|----------|-------------|
| [CONCAT](#concat) | Concatenates all arguments into a single string |
| [CONTAINS](#contains) | Checks if a string contains a substring |
| [ENDS_WITH](#ends_with) | Checks if a string ends with a suffix |
| [LENGTH](#length) | Returns the length of a string, array, or object |
| [LEVENSHTEIN_DISTANCE](#levenshtein_distance) | Calculates the Levenshtein (edit) distance between two strings |
| [LOWER](#lower) | Converts a string to lowercase |
| [LTRIM](#ltrim) | Removes leading whitespace or specified characters |
| [REGEX_REPLACE](#regex_replace) | Replaces matches of a regular expression |
| [REGEX_TEST](#regex_test) | Tests if a string matches a regular expression |
| [REPLACE](#replace) | Replaces all occurrences of a search string |
| [REVERSE](#reverse) | Reverses a string |
| [RTRIM](#rtrim) | Removes trailing whitespace or specified characters |
| [SPLIT](#split) | Splits a string into an array by a separator |
| [STARTS_WITH](#starts_with) | Checks if a string starts with a prefix |
| [SUBSTRING](#substring) | Extracts a substring from a string |
| [TRIM](#trim) | Removes leading and trailing whitespace or specified characters |
| [UPPER](#upper) | Converts a string to uppercase |

### CONCAT

**Signature:** `CONCAT(values)` → `string`  

Concatenates all arguments into a single string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | ✅ | Values to concatenate (variadic) |

**Examples:**

```aql
CONCAT("Hello", " ", "World") // "Hello World"
```

*Source: `string_functions.h`*

---

### CONTAINS

**Signature:** `CONTAINS(str, search)` → `boolean`  

Checks if a string contains a substring

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to search in |
| `search` | `string` | ✅ | Substring to find |

**Examples:**

```aql
CONTAINS("Hello World", "World") // true
```

*Source: `string_functions.h`*

---

### ENDS_WITH

**Signature:** `ENDS_WITH(str, suffix)` → `boolean`  

Checks if a string ends with a suffix

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to check |
| `suffix` | `string` | ✅ | Suffix to find |

**Examples:**

```aql
ENDS_WITH("Hello World", "World") // true
```

*Source: `string_functions.h`*

---

### LENGTH

**Signature:** `LENGTH(value)` → `integer`  

Returns the length of a string, array, or object

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | String, array, or object |

**Examples:**

```aql
LENGTH("hello") // 5
LENGTH([1, 2, 3]) // 3
```

*Source: `string_functions.h`*

---

### LEVENSHTEIN_DISTANCE

**Signature:** `LEVENSHTEIN_DISTANCE(str1, str2)` → `integer`  

Calculates the Levenshtein (edit) distance between two strings

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str1` | `string` | ✅ | First string |
| `str2` | `string` | ✅ | Second string |

**Examples:**

```aql
LEVENSHTEIN_DISTANCE("hello", "hallo") // 1
```

*Source: `string_functions.h`*

---

### LOWER

**Signature:** `LOWER(str)` → `string`  

Converts a string to lowercase

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to convert |

**Examples:**

```aql
LOWER("HELLO") // "hello"
```

*Source: `string_functions.h`*

---

### LTRIM

**Signature:** `LTRIM(str, chars?)` → `string`  

Removes leading whitespace or specified characters

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to trim |
| `chars` | `string` | — | Characters to remove |

**Examples:**

```aql
LTRIM("  hello") // "hello"
```

*Source: `string_functions.h`*

---

### REGEX_REPLACE

**Signature:** `REGEX_REPLACE(str, pattern, replacement)` → `string`  

Replaces matches of a regular expression

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | Source string |
| `pattern` | `string` | ✅ | Regular expression pattern |
| `replacement` | `string` | ✅ | Replacement string |

**Examples:**

```aql
REGEX_REPLACE("hello123world", "\\d+", "-") // "hello-world"
```

*Source: `string_functions.h`*

---

### REGEX_TEST

**Signature:** `REGEX_TEST(str, pattern)` → `boolean`  

Tests if a string matches a regular expression

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to test |
| `pattern` | `string` | ✅ | Regular expression pattern |

**Examples:**

```aql
REGEX_TEST("hello123", "\\d+") // true
```

*Source: `string_functions.h`*

---

### REPLACE

**Signature:** `REPLACE(str, search, replace)` → `string`  

Replaces all occurrences of a search string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | Source string |
| `search` | `string` | ✅ | String to find |
| `replace` | `string` | ✅ | Replacement string |

**Examples:**

```aql
REPLACE("Hello World", "World", "ThemisDB") // "Hello ThemisDB"
```

*Source: `string_functions.h`*

---

### REVERSE

**Signature:** `REVERSE(str)` → `string`  

Reverses a string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to reverse |

**Examples:**

```aql
REVERSE("hello") // "olleh"
```

*Source: `string_functions.h`*

---

### RTRIM

**Signature:** `RTRIM(str, chars?)` → `string`  

Removes trailing whitespace or specified characters

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to trim |
| `chars` | `string` | — | Characters to remove |

**Examples:**

```aql
RTRIM("hello  ") // "hello"
```

*Source: `string_functions.h`*

---

### SPLIT

**Signature:** `SPLIT(str, separator, limit?)` → `array`  

Splits a string into an array by a separator

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to split |
| `separator` | `string` | ✅ | Separator string |
| `limit` | `integer` | — | Maximum number of splits |

**Examples:**

```aql
SPLIT("a,b,c", ",") // ["a", "b", "c"]
```

*Source: `string_functions.h`*

---

### STARTS_WITH

**Signature:** `STARTS_WITH(str, prefix)` → `boolean`  

Checks if a string starts with a prefix

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to check |
| `prefix` | `string` | ✅ | Prefix to find |

**Examples:**

```aql
STARTS_WITH("Hello World", "Hello") // true
```

*Source: `string_functions.h`*

---

### SUBSTRING

**Signature:** `SUBSTRING(str, start, length?)` → `string`  

Extracts a substring from a string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | Source string |
| `start` | `integer` | ✅ | Start position (0-based) |
| `length` | `integer` | — | Number of characters (optional) |

**Examples:**

```aql
SUBSTRING("Hello World", 6) // "World"
SUBSTRING("Hello World", 0, 5) // "Hello"
```

*Source: `string_functions.h`*

---

### TRIM

**Signature:** `TRIM(str, chars?)` → `string`  

Removes leading and trailing whitespace or specified characters

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to trim |
| `chars` | `string` | — | Characters to remove |

**Examples:**

```aql
TRIM("  hello  ") // "hello"
TRIM("xxhelloxx", "x") // "hello"
```

*Source: `string_functions.h`*

---

### UPPER

**Signature:** `UPPER(str)` → `string`  

Converts a string to uppercase

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | ✅ | String to convert |

**Examples:**

```aql
UPPER("hello") // "HELLO"
```

*Source: `string_functions.h`*

---

## Math Functions

| Function | Description |
|----------|-------------|
| [ABS](#abs) | Returns the absolute value of a number |
| [ACOS](#acos) | Returns the arccosine of a number (result in radians) |
| [ASIN](#asin) | Returns the arcsine of a number (result in radians) |
| [ATAN](#atan) | Returns the arctangent of a number (result in radians) |
| [ATAN2](#atan2) | Returns the arctangent of y/x (result in radians) |
| [AVG](#avg) | Returns the average of all values in an array |
| [CEIL](#ceil) | Returns the smallest integer greater than or equal to a number |
| [COS](#cos) | Returns the cosine of an angle (in radians) |
| [DEGREES](#degrees) | Converts radians to degrees |
| [EXP](#exp) | Returns e raised to the power of a number |
| [FLOOR](#floor) | Returns the largest integer less than or equal to a number |
| [LOG](#log) | Returns the logarithm of a number (natural log by default) |
| [LOG10](#log10) | Returns the base-10 logarithm of a number |
| [MAX](#max) | Returns the maximum value from arguments or array |
| [MIN](#min) | Returns the minimum value from arguments or array |
| [PI](#pi) | Returns the value of Pi |
| [POW](#pow) | Returns base raised to the power of exponent |
| [RADIANS](#radians) | Converts degrees to radians |
| [RANDOM](#random) | Returns a random number between 0 and 1 |
| [RAND_INT](#rand_int) | Returns a random integer between min and max (inclusive) |
| [ROUND](#round) | Rounds a number to a specified precision |
| [SIN](#sin) | Returns the sine of an angle (in radians) |
| [SQRT](#sqrt) | Returns the square root of a number |
| [SUM](#sum) | Returns the sum of all values in an array |
| [TAN](#tan) | Returns the tangent of an angle (in radians) |

### ABS

**Signature:** `ABS(num)` → `number`  

Returns the absolute value of a number

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Number |

**Examples:**

```aql
ABS(-5) // 5
```

*Source: `math_functions.h`*

---

### ACOS

**Signature:** `ACOS(num)` → `number`  

Returns the arccosine of a number (result in radians)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Number between -1 and 1 |

**Examples:**

```aql
ACOS(1) // 0
```

*Source: `math_functions.h`*

---

### ASIN

**Signature:** `ASIN(num)` → `number`  

Returns the arcsine of a number (result in radians)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Number between -1 and 1 |

**Examples:**

```aql
ASIN(0) // 0
```

*Source: `math_functions.h`*

---

### ATAN

**Signature:** `ATAN(num)` → `number`  

Returns the arctangent of a number (result in radians)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Number |

**Examples:**

```aql
ATAN(0) // 0
```

*Source: `math_functions.h`*

---

### ATAN2

**Signature:** `ATAN2(y, x)` → `number`  

Returns the arctangent of y/x (result in radians)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `y` | `number` | ✅ | Y coordinate |
| `x` | `number` | ✅ | X coordinate |

**Examples:**

```aql
ATAN2(1, 1) // 0.785...
```

*Source: `math_functions.h`*

---

### AVG

**Signature:** `AVG(array)` → `number`    
**Aggregate:** ✅

Returns the average of all values in an array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of numbers |

**Examples:**

```aql
AVG([1, 2, 3, 4]) // 2.5
```

*Source: `math_functions.h`*

---

### CEIL

**Signature:** `CEIL(num)` → `integer`  

Returns the smallest integer greater than or equal to a number

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Number |

**Examples:**

```aql
CEIL(4.3) // 5
```

*Source: `math_functions.h`*

---

### COS

**Signature:** `COS(angle)` → `number`  

Returns the cosine of an angle (in radians)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `angle` | `number` | ✅ | Angle in radians |

**Examples:**

```aql
COS(0) // 1
```

*Source: `math_functions.h`*

---

### DEGREES

**Signature:** `DEGREES(radians)` → `number`  

Converts radians to degrees

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `radians` | `number` | ✅ | Angle in radians |

**Examples:**

```aql
DEGREES(3.14159) // 180
```

*Source: `math_functions.h`*

---

### EXP

**Signature:** `EXP(num)` → `number`  

Returns e raised to the power of a number

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Exponent |

**Examples:**

```aql
EXP(1) // 2.718...
```

*Source: `math_functions.h`*

---

### FLOOR

**Signature:** `FLOOR(num)` → `integer`  

Returns the largest integer less than or equal to a number

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Number |

**Examples:**

```aql
FLOOR(4.7) // 4
```

*Source: `math_functions.h`*

---

### LOG

**Signature:** `LOG(num, base?)` → `number`  

Returns the logarithm of a number (natural log by default)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Positive number |
| `base` | `number` | — | Base (default: e) |

**Examples:**

```aql
LOG(10) // 2.302...
LOG(100, 10) // 2
```

*Source: `math_functions.h`*

---

### LOG10

**Signature:** `LOG10(num)` → `number`  

Returns the base-10 logarithm of a number

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Positive number |

**Examples:**

```aql
LOG10(100) // 2
```

*Source: `math_functions.h`*

---

### MAX

**Signature:** `MAX(values)` → `number`  

Returns the maximum value from arguments or array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | ✅ | Numbers or array |

**Examples:**

```aql
MAX(1, 2, 3) // 3
MAX([5, 2, 8]) // 8
```

*Source: `math_functions.h`*

---

### MIN

**Signature:** `MIN(values)` → `number`  

Returns the minimum value from arguments or array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | ✅ | Numbers or array |

**Examples:**

```aql
MIN(1, 2, 3) // 1
MIN([5, 2, 8]) // 2
```

*Source: `math_functions.h`*

---

### PI

**Signature:** `PI()` → `number`  

Returns the value of Pi

**Examples:**

```aql
PI() // 3.14159...
```

*Source: `math_functions.h`*

---

### POW

**Signature:** `POW(base, exponent)` → `number`  

Returns base raised to the power of exponent

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `base` | `number` | ✅ | Base number |
| `exponent` | `number` | ✅ | Exponent |

**Examples:**

```aql
POW(2, 3) // 8
```

*Source: `math_functions.h`*

---

### RADIANS

**Signature:** `RADIANS(degrees)` → `number`  

Converts degrees to radians

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `degrees` | `number` | ✅ | Angle in degrees |

**Examples:**

```aql
RADIANS(180) // 3.14159...
```

*Source: `math_functions.h`*

---

### RANDOM

**Signature:** `RANDOM()` → `number`    
**Non-deterministic** (result may vary)

Returns a random number between 0 and 1

**Examples:**

```aql
RANDOM() // 0.7234...
```

*Source: `math_functions.h`*

---

### RAND_INT

**Signature:** `RAND_INT(min, max)` → `integer`    
**Non-deterministic** (result may vary)

Returns a random integer between min and max (inclusive)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `min` | `integer` | ✅ | Minimum value |
| `max` | `integer` | ✅ | Maximum value |

**Examples:**

```aql
RAND_INT(1, 100) // 42
```

*Source: `math_functions.h`*

---

### ROUND

**Signature:** `ROUND(num, precision?)` → `number`  

Rounds a number to a specified precision

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Number to round |
| `precision` | `integer` | — | Decimal places (default: 0) |

**Examples:**

```aql
ROUND(4.567) // 5
ROUND(4.567, 2) // 4.57
```

*Source: `math_functions.h`*

---

### SIN

**Signature:** `SIN(angle)` → `number`  

Returns the sine of an angle (in radians)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `angle` | `number` | ✅ | Angle in radians |

**Examples:**

```aql
SIN(0) // 0
```

*Source: `math_functions.h`*

---

### SQRT

**Signature:** `SQRT(num)` → `number`  

Returns the square root of a number

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `num` | `number` | ✅ | Non-negative number |

**Examples:**

```aql
SQRT(16) // 4
```

*Source: `math_functions.h`*

---

### SUM

**Signature:** `SUM(array)` → `number`    
**Aggregate:** ✅

Returns the sum of all values in an array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of numbers |

**Examples:**

```aql
SUM([1, 2, 3, 4]) // 10
```

*Source: `math_functions.h`*

---

### TAN

**Signature:** `TAN(angle)` → `number`  

Returns the tangent of an angle (in radians)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `angle` | `number` | ✅ | Angle in radians |

**Examples:**

```aql
TAN(0) // 0
```

*Source: `math_functions.h`*

---

## Array Functions

| Function | Description |
|----------|-------------|
| [COUNT](#count) | Returns the number of elements in an array or 1 for non-arrays |
| [FIRST](#first) | Returns the first element of an array |
| [FLATTEN](#flatten) | Flattens nested arrays to a single level |
| [INTERSECTION](#intersection) | Returns the intersection of all arrays (common values) |
| [LAST](#last) | Returns the last element of an array |
| [MINUS](#minus) | Returns elements in first array that are not in second |
| [NTH](#nth) | Returns the element at a specific index (0-based) |
| [POP](#pop) | Returns the array with the last element removed |
| [POSITION](#position) | Returns the index of a value in an array (-1 if not found) |
| [PUSH](#push) | Appends a value to an array and returns the new array |
| [RANGE](#range) | Generates an array of numbers from start to end |
| [REVERSE_ARRAY](#reverse_array) | Returns a reversed copy of the array |
| [SHIFT](#shift) | Returns the array with the first element removed |
| [SLICE](#slice) | Returns a portion of an array |
| [SORTED](#sorted) | Returns a sorted copy of the array |
| [UNION](#union) | Returns the union of all arrays (unique values) |
| [UNIQUE](#unique) | Returns an array with duplicate values removed |
| [UNSHIFT](#unshift) | Prepends a value to an array and returns the new array |

### COUNT

**Signature:** `COUNT(value)` → `integer`    
**Aggregate:** ✅

Returns the number of elements in an array or 1 for non-arrays

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Array or value |

**Examples:**

```aql
COUNT([1, 2, 3]) // 3
COUNT(null) // 0
```

*Source: `array_functions.h`*

---

### FIRST

**Signature:** `FIRST(array)` → `any`  

Returns the first element of an array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |

**Examples:**

```aql
FIRST([1, 2, 3]) // 1
```

*Source: `array_functions.h`*

---

### FLATTEN

**Signature:** `FLATTEN(array, depth?)` → `array`  

Flattens nested arrays to a single level

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |
| `depth` | `integer` | — | Maximum depth to flatten |

**Examples:**

```aql
FLATTEN([[1, 2], [3, 4]]) // [1, 2, 3, 4]
FLATTEN([[[1]], [[2]]], 2) // [1, 2]
```

*Source: `array_functions.h`*

---

### INTERSECTION

**Signature:** `INTERSECTION(arrays)` → `array`  

Returns the intersection of all arrays (common values)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `arrays` | `array` | ✅ | Arrays to intersect (variadic) |

**Examples:**

```aql
INTERSECTION([1, 2, 3], [2, 3, 4]) // [2, 3]
```

*Source: `array_functions.h`*

---

### LAST

**Signature:** `LAST(array)` → `any`  

Returns the last element of an array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |

**Examples:**

```aql
LAST([1, 2, 3]) // 3
```

*Source: `array_functions.h`*

---

### MINUS

**Signature:** `MINUS(arr1, arr2)` → `array`  

Returns elements in first array that are not in second

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `arr1` | `array` | ✅ | First array |
| `arr2` | `array` | ✅ | Second array |

**Examples:**

```aql
MINUS([1, 2, 3], [2, 4]) // [1, 3]
```

*Source: `array_functions.h`*

---

### NTH

**Signature:** `NTH(array, index)` → `any`  

Returns the element at a specific index (0-based)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |
| `index` | `integer` | ✅ | Index (0-based) |

**Examples:**

```aql
NTH([1, 2, 3], 1) // 2
```

*Source: `array_functions.h`*

---

### POP

**Signature:** `POP(array)` → `array`  

Returns the array with the last element removed

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |

**Examples:**

```aql
POP([1, 2, 3]) // [1, 2]
```

*Source: `array_functions.h`*

---

### POSITION

**Signature:** `POSITION(array, value)` → `integer`  

Returns the index of a value in an array (-1 if not found)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |
| `value` | `any` | ✅ | Value to find |

**Examples:**

```aql
POSITION([1, 2, 3], 2) // 1
```

*Source: `array_functions.h`*

---

### PUSH

**Signature:** `PUSH(array, value, unique?)` → `array`  

Appends a value to an array and returns the new array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |
| `value` | `any` | ✅ | Value to append |
| `unique` | `boolean` | — | Only add if not already present |

**Examples:**

```aql
PUSH([1, 2], 3) // [1, 2, 3]
PUSH([1, 2], 2, true) // [1, 2]
```

*Source: `array_functions.h`*

---

### RANGE

**Signature:** `RANGE(start, end, step?)` → `array`  

Generates an array of numbers from start to end

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `start` | `integer` | ✅ | Start value (inclusive) |
| `end` | `integer` | ✅ | End value (inclusive) |
| `step` | `integer` | — | Step size |

**Examples:**

```aql
RANGE(1, 5) // [1, 2, 3, 4, 5]
RANGE(0, 10, 2) // [0, 2, 4, 6, 8, 10]
```

*Source: `array_functions.h`*

---

### REVERSE_ARRAY

**Signature:** `REVERSE_ARRAY(array)` → `array`  

Returns a reversed copy of the array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |

**Examples:**

```aql
REVERSE_ARRAY([1, 2, 3]) // [3, 2, 1]
```

*Source: `array_functions.h`*

---

### SHIFT

**Signature:** `SHIFT(array)` → `array`  

Returns the array with the first element removed

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |

**Examples:**

```aql
SHIFT([1, 2, 3]) // [2, 3]
```

*Source: `array_functions.h`*

---

### SLICE

**Signature:** `SLICE(array, start, end?)` → `array`  

Returns a portion of an array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |
| `start` | `integer` | ✅ | Start index (inclusive) |
| `end` | `integer` | — | End index (exclusive) |

**Examples:**

```aql
SLICE([1, 2, 3, 4], 1, 3) // [2, 3]
SLICE([1, 2, 3, 4], 2) // [3, 4]
```

*Source: `array_functions.h`*

---

### SORTED

**Signature:** `SORTED(array, direction?)` → `array`  

Returns a sorted copy of the array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |
| `direction` | `string` | — | Sort direction: ASC or DESC |

**Examples:**

```aql
SORTED([3, 1, 2]) // [1, 2, 3]
SORTED([1, 2, 3], "DESC") // [3, 2, 1]
```

*Source: `array_functions.h`*

---

### UNION

**Signature:** `UNION(arrays)` → `array`  

Returns the union of all arrays (unique values)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `arrays` | `array` | ✅ | Arrays to combine (variadic) |

**Examples:**

```aql
UNION([1, 2], [2, 3]) // [1, 2, 3]
```

*Source: `array_functions.h`*

---

### UNIQUE

**Signature:** `UNIQUE(array)` → `array`  

Returns an array with duplicate values removed

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |

**Examples:**

```aql
UNIQUE([1, 2, 1, 3, 2]) // [1, 2, 3]
```

*Source: `array_functions.h`*

---

### UNSHIFT

**Signature:** `UNSHIFT(array, value)` → `array`  

Prepends a value to an array and returns the new array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Input array |
| `value` | `any` | ✅ | Value to prepend |

**Examples:**

```aql
UNSHIFT([2, 3], 1) // [1, 2, 3]
```

*Source: `array_functions.h`*

---

## Date Functions

| Function | Description |
|----------|-------------|
| [AGE](#age) | Calculates the age in complete years between two dates |
| [CURRENT_DATE](#current_date) | SQL-compatible: Returns current date as timestamp at 00:00:00 UTC |
| [CURRENT_TIME](#current_time) | SQL-compatible: Returns current time as milliseconds since midnight UTC |
| [CURRENT_TIMESTAMP](#current_timestamp) | SQL-compatible: Returns current Unix timestamp in milliseconds |
| [DATE_ADD](#date_add) | Adds a time amount to a timestamp |
| [DATE_BETWEEN](#date_between) | Returns true if date is between start and end (inclusive) |
| [DATE_COMPARE](#date_compare) | Compares two timestamps: returns -1, 0, or 1 |
| [DATE_DAY](#date_day) | Extracts the day of month (1-31) from a timestamp |
| [DATE_DAYOFWEEK](#date_dayofweek) | Returns the day of week (0=Sunday, 6=Saturday) |
| [DATE_DAYOFYEAR](#date_dayofyear) | Returns the day of year (1-366) |
| [DATE_DAYS_IN_MONTH](#date_days_in_month) | Returns the number of days in the specified month |
| [DATE_DIFF](#date_diff) | Returns the difference between two timestamps |
| [DATE_END_OF_MONTH](#date_end_of_month) | Returns the timestamp of the last day of the month (23:59:59.999) |
| [DATE_FORMAT](#date_format) | Formats a timestamp using a format pattern |
| [DATE_HOUR](#date_hour) | Extracts the hour (0-23) from a timestamp |
| [DATE_ISO8601](#date_iso8601) | Formats a Unix timestamp as an ISO 8601 date string |
| [DATE_LEAPYEAR](#date_leapyear) | Returns true if the year is a leap year |
| [DATE_MILLISECOND](#date_millisecond) | Extracts the millisecond (0-999) from a timestamp |
| [DATE_MINUTE](#date_minute) | Extracts the minute (0-59) from a timestamp |
| [DATE_MONTH](#date_month) | Extracts the month (1-12) from a timestamp |
| [DATE_NOW](#date_now) | Returns the current Unix timestamp in milliseconds |
| [DATE_QUARTER](#date_quarter) | Returns the quarter of the year (1-4) |
| [DATE_SECOND](#date_second) | Extracts the second (0-59) from a timestamp |
| [DATE_START_OF_WEEK](#date_start_of_week) | Returns the timestamp of the start of the week |
| [DATE_SUB](#date_sub) | Alias for DATE_SUBTRACT - subtracts time from a timestamp |
| [DATE_SUBTRACT](#date_subtract) | Subtracts a time amount from a timestamp |
| [DATE_TIMESTAMP](#date_timestamp) | Converts an ISO 8601 date string to a Unix timestamp |
| [DATE_TRUNC](#date_trunc) | Truncates a timestamp to the specified unit boundary |
| [DATE_WEEK](#date_week) | Returns the ISO week number (1-53) |
| [DATE_YEAR](#date_year) | Extracts the year from a timestamp |
| [DAYS](#days) | Creates an interval of N days in milliseconds |
| [EPOCH_MS](#epoch_ms) | Returns timestamp as epoch milliseconds (identity function) |
| [EPOCH_SECONDS](#epoch_seconds) | Converts millisecond timestamp to epoch seconds |
| [FROM_UNIXTIME](#from_unixtime) | MySQL compatible: Converts Unix seconds to millisecond timestamp |
| [GETDATE](#getdate) | SQL Server compatible: Returns current Unix timestamp in milliseconds |
| [HOURS](#hours) | Creates an interval of N hours in milliseconds |
| [INTERVAL](#interval) | Creates a time interval in milliseconds for date arithmetic |
| [IS_WEEKEND](#is_weekend) | Returns true if the date falls on Saturday or Sunday |
| [IS_WORKDAY](#is_workday) | Returns true if the date is a business day (Mon-Fri, not a holiday) |
| [MAKE_DATE](#make_date) | Creates a timestamp from year, month, day components |
| [MAKE_DATETIME](#make_datetime) | Creates a timestamp from date and time components |
| [MAKE_TIME](#make_time) | Creates a time value as milliseconds since midnight |
| [MINUTES](#minutes) | Creates an interval of N minutes in milliseconds |
| [MONTHS](#months) | Creates an interval of N months in milliseconds |
| [NOW](#now) | SQL-compatible: Returns current Unix timestamp in milliseconds |
| [SECONDS](#seconds) | Creates an interval of N seconds in milliseconds |
| [SYSDATE](#sysdate) | Oracle compatible: Returns current Unix timestamp in milliseconds |
| [TODAY](#today) | Returns start of today (00:00:00 UTC) as timestamp |
| [TOMORROW](#tomorrow) | Returns start of tomorrow (00:00:00 UTC) as timestamp |
| [UNIX_TIMESTAMP](#unix_timestamp) | MySQL compatible: Returns current Unix timestamp in seconds |
| [WEEKS](#weeks) | Creates an interval of N weeks in milliseconds |
| [WORKDAYS](#workdays) | Counts business days (Mon-Fri) between two dates, optionally excluding holidays |
| [WORKDAYS_ADD](#workdays_add) | Adds N business days to a date, skipping weekends and holidays |
| [YEARS](#years) | Creates an interval of N years in milliseconds |
| [YESTERDAY](#yesterday) | Returns start of yesterday (00:00:00 UTC) as timestamp |

### AGE

**Signature:** `AGE(birthdate, referenceDate?)` → `integer`    
**Non-deterministic** (result may vary)

Calculates the age in complete years between two dates

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `birthdate` | `integer` | ✅ | Birth date timestamp in ms |
| `referenceDate` | `integer` | — | Reference date (default: now) |

**Examples:**

```aql
AGE(DATE_TIMESTAMP("1990-05-15")) // Age today
AGE(birthdate, DATE_TIMESTAMP("2020-01-01")) // Age on specific date
```

*Source: `date_functions.h`*

---

### CURRENT_DATE

**Signature:** `CURRENT_DATE()` → `integer`    
**Non-deterministic** (result may vary)

SQL-compatible: Returns current date as timestamp at 00:00:00 UTC

**Examples:**

```aql
CURRENT_DATE() // Start of today
CURRENT_DATE() + DAYS(1) // Start of tomorrow
```

*Source: `date_functions.h`*

---

### CURRENT_TIME

**Signature:** `CURRENT_TIME()` → `integer`    
**Non-deterministic** (result may vary)

SQL-compatible: Returns current time as milliseconds since midnight UTC

**Examples:**

```aql
CURRENT_TIME() // e.g., 52200000 for 14:30:00
CURRENT_TIME() / 3600000 // Current hour
```

*Source: `date_functions.h`*

---

### CURRENT_TIMESTAMP

**Signature:** `CURRENT_TIMESTAMP()` → `integer`    
**Non-deterministic** (result may vary)

SQL-compatible: Returns current Unix timestamp in milliseconds

**Examples:**

```aql
CURRENT_TIMESTAMP() // 1700000000000
```

*Source: `date_functions.h`*

---

### DATE_ADD

**Signature:** `DATE_ADD(timestamp, amount, unit)` → `integer`  

Adds a time amount to a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |
| `amount` | `integer` | ✅ | Amount to add |
| `unit` | `string` | ✅ | Unit: year, month, day, hour, minute, second, millisecond |

**Examples:**

```aql
DATE_ADD(1700000000000, 7, "day")
```

*Source: `date_functions.h`*

---

### DATE_BETWEEN

**Signature:** `DATE_BETWEEN(date, startDate, endDate)` → `boolean`  

Returns true if date is between start and end (inclusive)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `date` | `integer` | ✅ | Date to check |
| `startDate` | `integer` | ✅ | Range start |
| `endDate` | `integer` | ✅ | Range end |

**Examples:**

```aql
DATE_BETWEEN(event.date, MAKE_DATE(2024,1,1), MAKE_DATE(2024,12,31))
DATE_BETWEEN(NOW(), startTime, endTime)
```

*Source: `date_functions.h`*

---

### DATE_COMPARE

**Signature:** `DATE_COMPARE(date1, date2)` → `integer`  

Compares two timestamps: returns -1, 0, or 1

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `date1` | `integer` | ✅ | First timestamp in ms |
| `date2` | `integer` | ✅ | Second timestamp in ms |

**Examples:**

```aql
DATE_COMPARE(NOW(), YESTERDAY()) // 1 (now is after yesterday)
DATE_COMPARE(event1.date, event2.date)
```

*Source: `date_functions.h`*

---

### DATE_DAY

**Signature:** `DATE_DAY(timestamp)` → `integer`  

Extracts the day of month (1-31) from a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_DAY(1700000000000) // 14
```

*Source: `date_functions.h`*

---

### DATE_DAYOFWEEK

**Signature:** `DATE_DAYOFWEEK(timestamp)` → `integer`  

Returns the day of week (0=Sunday, 6=Saturday)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_DAYOFWEEK(1700000000000) // 2 (Tuesday)
```

*Source: `date_functions.h`*

---

### DATE_DAYOFYEAR

**Signature:** `DATE_DAYOFYEAR(timestamp)` → `integer`  

Returns the day of year (1-366)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_DAYOFYEAR(1700000000000) // 318
```

*Source: `date_functions.h`*

---

### DATE_DAYS_IN_MONTH

**Signature:** `DATE_DAYS_IN_MONTH(year, month)` → `integer`  

Returns the number of days in the specified month

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `year` | `integer` | ✅ | Year |
| `month` | `integer` | ✅ | Month (1-12) |

**Examples:**

```aql
DATE_DAYS_IN_MONTH(2024, 2) // 29 (leap year)
DATE_DAYS_IN_MONTH(2023, 2) // 28
```

*Source: `date_functions.h`*

---

### DATE_DIFF

**Signature:** `DATE_DIFF(timestamp1, timestamp2, unit)` → `number`  

Returns the difference between two timestamps

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp1` | `integer` | ✅ | First timestamp in ms |
| `timestamp2` | `integer` | ✅ | Second timestamp in ms |
| `unit` | `string` | ✅ | Unit for result: day, hour, minute, second, millisecond |

**Examples:**

```aql
DATE_DIFF(ts2, ts1, "day") // days between
```

*Source: `date_functions.h`*

---

### DATE_END_OF_MONTH

**Signature:** `DATE_END_OF_MONTH(timestamp)` → `integer`  

Returns the timestamp of the last day of the month (23:59:59.999)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_END_OF_MONTH(DATE_NOW())
```

*Source: `date_functions.h`*

---

### DATE_FORMAT

**Signature:** `DATE_FORMAT(timestamp, format)` → `string`  

Formats a timestamp using a format pattern

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |
| `format` | `string` | ✅ | Format pattern (strftime) |

**Examples:**

```aql
DATE_FORMAT(1700000000000, "%Y-%m-%d") // "2023-11-14"
```

*Source: `date_functions.h`*

---

### DATE_HOUR

**Signature:** `DATE_HOUR(timestamp)` → `integer`  

Extracts the hour (0-23) from a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_HOUR(1700000000000) // 22
```

*Source: `date_functions.h`*

---

### DATE_ISO8601

**Signature:** `DATE_ISO8601(timestamp)` → `string`  

Formats a Unix timestamp as an ISO 8601 date string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_ISO8601(1700000000000) // "2023-11-14T22:13:20Z"
```

*Source: `date_functions.h`*

---

### DATE_LEAPYEAR

**Signature:** `DATE_LEAPYEAR(year)` → `boolean`  

Returns true if the year is a leap year

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `year` | `integer` | ✅ | Year to check (e.g., 2024) |

**Examples:**

```aql
DATE_LEAPYEAR(2024) // true
DATE_LEAPYEAR(2023) // false
```

*Source: `date_functions.h`*

---

### DATE_MILLISECOND

**Signature:** `DATE_MILLISECOND(timestamp)` → `integer`  

Extracts the millisecond (0-999) from a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_MILLISECOND(1700000000123) // 123
```

*Source: `date_functions.h`*

---

### DATE_MINUTE

**Signature:** `DATE_MINUTE(timestamp)` → `integer`  

Extracts the minute (0-59) from a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_MINUTE(1700000000000) // 13
```

*Source: `date_functions.h`*

---

### DATE_MONTH

**Signature:** `DATE_MONTH(timestamp)` → `integer`  

Extracts the month (1-12) from a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_MONTH(1700000000000) // 11
```

*Source: `date_functions.h`*

---

### DATE_NOW

**Signature:** `DATE_NOW()` → `integer`    
**Non-deterministic** (result may vary)

Returns the current Unix timestamp in milliseconds

**Examples:**

```aql
DATE_NOW() // 1700000000000
```

*Source: `date_functions.h`*

---

### DATE_QUARTER

**Signature:** `DATE_QUARTER(timestamp)` → `integer`  

Returns the quarter of the year (1-4)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_QUARTER(DATE_TIMESTAMP("2024-08-15")) // 3
```

*Source: `date_functions.h`*

---

### DATE_SECOND

**Signature:** `DATE_SECOND(timestamp)` → `integer`  

Extracts the second (0-59) from a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_SECOND(1700000000000) // 20
```

*Source: `date_functions.h`*

---

### DATE_START_OF_WEEK

**Signature:** `DATE_START_OF_WEEK(timestamp, startDay?)` → `integer`  

Returns the timestamp of the start of the week

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |
| `startDay` | `integer` | — | First day of week (0=Sun, 1=Mon). Default: 1 (Monday) |

**Examples:**

```aql
DATE_START_OF_WEEK(DATE_NOW()) // Monday of current week
DATE_START_OF_WEEK(DATE_NOW(), 0) // Sunday of current week
```

*Source: `date_functions.h`*

---

### DATE_SUB

**Signature:** `DATE_SUB(timestamp, amount, unit)` → `integer`  

Alias for DATE_SUBTRACT - subtracts time from a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |
| `amount` | `integer` | ✅ | Amount to subtract |
| `unit` | `string` | ✅ | Unit: year, month, day, hour, minute, second |

**Examples:**

```aql
DATE_SUB(NOW(), 1, 'year') // One year ago
DATE_SUB(NOW(), 7, 'days') // One week ago
DATE_SUB(NOW(), 90, 'days') // Three months ago
```

*Source: `retention_functions.h`*

---

### DATE_SUBTRACT

**Signature:** `DATE_SUBTRACT(timestamp, amount, unit)` → `integer`  

Subtracts a time amount from a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |
| `amount` | `integer` | ✅ | Amount to subtract |
| `unit` | `string` | ✅ | Unit: year, month, day, hour, minute, second, millisecond |

**Examples:**

```aql
DATE_SUBTRACT(1700000000000, 1, "month")
```

*Source: `date_functions.h`*

---

### DATE_TIMESTAMP

**Signature:** `DATE_TIMESTAMP(dateStr)` → `integer`  

Converts an ISO 8601 date string to a Unix timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `dateStr` | `string` | ✅ | ISO 8601 date string |

**Examples:**

```aql
DATE_TIMESTAMP("2024-01-15T10:30:00Z")
```

*Source: `date_functions.h`*

---

### DATE_TRUNC

**Signature:** `DATE_TRUNC(timestamp, unit)` → `integer`  

Truncates a timestamp to the specified unit boundary

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |
| `unit` | `string` | ✅ | Unit: year, month, day, hour, minute, second |

**Examples:**

```aql
DATE_TRUNC(1700000000000, "day") // Start of day
```

*Source: `date_functions.h`*

---

### DATE_WEEK

**Signature:** `DATE_WEEK(timestamp)` → `integer`  

Returns the ISO week number (1-53)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_WEEK(DATE_TIMESTAMP("2024-01-15")) // 3
```

*Source: `date_functions.h`*

---

### DATE_YEAR

**Signature:** `DATE_YEAR(timestamp)` → `integer`  

Extracts the year from a timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
DATE_YEAR(1700000000000) // 2023
```

*Source: `date_functions.h`*

---

### DAYS

**Signature:** `DAYS(amount)` → `integer`  

Creates an interval of N days in milliseconds

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `amount` | `number` | ✅ | Number of days (can be fractional or negative) |

**Examples:**

```aql
DAYS(7) // 604800000 ms
DATE_NOW() + DAYS(30)
DAYS(-14) // Two weeks ago
```

*Source: `date_functions.h`*

---

### EPOCH_MS

**Signature:** `EPOCH_MS(timestamp)` → `integer`  

Returns timestamp as epoch milliseconds (identity function)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
EPOCH_MS(DATE_NOW()) // Same as input
```

*Source: `date_functions.h`*

---

### EPOCH_SECONDS

**Signature:** `EPOCH_SECONDS(timestamp)` → `integer`  

Converts millisecond timestamp to epoch seconds

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
EPOCH_SECONDS(1700000000000) // 1700000000
```

*Source: `date_functions.h`*

---

### FROM_UNIXTIME

**Signature:** `FROM_UNIXTIME(seconds)` → `integer`  

MySQL compatible: Converts Unix seconds to millisecond timestamp

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `seconds` | `integer` | ✅ | Unix timestamp in seconds |

**Examples:**

```aql
FROM_UNIXTIME(1700000000) // 1700000000000
```

*Source: `date_functions.h`*

---

### GETDATE

**Signature:** `GETDATE()` → `integer`    
**Non-deterministic** (result may vary)

SQL Server compatible: Returns current Unix timestamp in milliseconds

**Examples:**

```aql
GETDATE() // 1700000000000
```

*Source: `date_functions.h`*

---

### HOURS

**Signature:** `HOURS(amount)` → `integer`  

Creates an interval of N hours in milliseconds

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `amount` | `number` | ✅ | Number of hours (can be fractional or negative) |

**Examples:**

```aql
HOURS(24) // 86400000 ms (1 day)
DATE_NOW() - HOURS(12)
HOURS(1.5) // 90 minutes
```

*Source: `date_functions.h`*

---

### INTERVAL

**Signature:** `INTERVAL(amount, unit)` → `integer`  

Creates a time interval in milliseconds for date arithmetic

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `amount` | `number` | ✅ | Numeric amount (can be negative) |
| `unit` | `string` | ✅ | Unit: years, months, weeks, days, hours, minutes, seconds, ms |

**Examples:**

```aql
INTERVAL(7, "days") // 604800000 ms
INTERVAL(2, "weeks") // 1209600000 ms
DATE_NOW() + INTERVAL(1, "month")
```

*Source: `date_functions.h`*

---

### IS_WEEKEND

**Signature:** `IS_WEEKEND(timestamp)` → `boolean`  

Returns true if the date falls on Saturday or Sunday

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |

**Examples:**

```aql
IS_WEEKEND(DATE_TIMESTAMP("2024-01-06")) // true (Saturday)
```

*Source: `date_functions.h`*

---

### IS_WORKDAY

**Signature:** `IS_WORKDAY(timestamp, holidays?)` → `boolean`  

Returns true if the date is a business day (Mon-Fri, not a holiday)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `timestamp` | `integer` | ✅ | Unix timestamp in ms |
| `holidays` | `array` | — | Optional array of holiday timestamps |

**Examples:**

```aql
IS_WORKDAY(DATE_TIMESTAMP("2024-01-08")) // true (Monday)
IS_WORKDAY(date, holidayArray)
```

*Source: `date_functions.h`*

---

### MAKE_DATE

**Signature:** `MAKE_DATE(year, month, day)` → `integer`  

Creates a timestamp from year, month, day components

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `year` | `integer` | ✅ | Year |
| `month` | `integer` | ✅ | Month (1-12) |
| `day` | `integer` | ✅ | Day (1-31) |

**Examples:**

```aql
MAKE_DATE(2024, 12, 25) // Christmas 2024
MAKE_DATE(DATE_YEAR(NOW()), 1, 1) // Start of this year
```

*Source: `date_functions.h`*

---

### MAKE_DATETIME

**Signature:** `MAKE_DATETIME(year, month, day, hour?, minute?, second?)` → `integer`  

Creates a timestamp from date and time components

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `year` | `integer` | ✅ | Year |
| `month` | `integer` | ✅ | Month (1-12) |
| `day` | `integer` | ✅ | Day (1-31) |
| `hour` | `integer` | — | Hour (0-23), default 0 |
| `minute` | `integer` | — | Minute (0-59), default 0 |
| `second` | `integer` | — | Second (0-59), default 0 |

**Examples:**

```aql
MAKE_DATETIME(2024, 12, 31, 23, 59, 59) // New Year's Eve countdown
MAKE_DATETIME(2024, 1, 1) // Midnight on New Year
```

*Source: `date_functions.h`*

---

### MAKE_TIME

**Signature:** `MAKE_TIME(hour, minute, second?)` → `integer`  

Creates a time value as milliseconds since midnight

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `hour` | `integer` | ✅ | Hour (0-23) |
| `minute` | `integer` | ✅ | Minute (0-59) |
| `second` | `integer` | — | Second (0-59), default 0 |

**Examples:**

```aql
MAKE_TIME(14, 30) // 52200000 (2:30 PM)
MAKE_TIME(9, 0, 0) // 32400000 (9:00 AM)
```

*Source: `date_functions.h`*

---

### MINUTES

**Signature:** `MINUTES(amount)` → `integer`  

Creates an interval of N minutes in milliseconds

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `amount` | `number` | ✅ | Number of minutes (can be fractional or negative) |

**Examples:**

```aql
MINUTES(60) // 3600000 ms (1 hour)
DATE_NOW() + MINUTES(30)
MINUTES(-15) // 15 minutes ago
```

*Source: `date_functions.h`*

---

### MONTHS

**Signature:** `MONTHS(amount)` → `integer`  

Creates an interval of N months in milliseconds

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `amount` | `number` | ✅ | Number of months (can be fractional or negative) |

**Examples:**

```aql
MONTHS(3) // 7889400000 ms (91.3125 days)
DATE_NOW() - MONTHS(6)
MONTHS(0.5) // Half a month
```

*Source: `date_functions.h`*

---

### NOW

**Signature:** `NOW()` → `integer`    
**Non-deterministic** (result may vary)

SQL-compatible: Returns current Unix timestamp in milliseconds

**Examples:**

```aql
NOW() // 1700000000000
NOW() - DAYS(7) // One week ago
```

*Source: `date_functions.h`*

---

### SECONDS

**Signature:** `SECONDS(amount)` → `integer`  

Creates an interval of N seconds in milliseconds

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `amount` | `number` | ✅ | Number of seconds (can be fractional or negative) |

**Examples:**

```aql
SECONDS(60) // 60000 ms (1 minute)
DATE_NOW() + SECONDS(30)
SECONDS(0.5) // 500 ms
```

*Source: `date_functions.h`*

---

### SYSDATE

**Signature:** `SYSDATE()` → `integer`    
**Non-deterministic** (result may vary)

Oracle compatible: Returns current Unix timestamp in milliseconds

**Examples:**

```aql
SYSDATE() // 1700000000000
```

*Source: `date_functions.h`*

---

### TODAY

**Signature:** `TODAY()` → `integer`    
**Non-deterministic** (result may vary)

Returns start of today (00:00:00 UTC) as timestamp

**Examples:**

```aql
TODAY() // Start of today
event.date >= TODAY() // Events today or later
```

*Source: `date_functions.h`*

---

### TOMORROW

**Signature:** `TOMORROW()` → `integer`    
**Non-deterministic** (result may vary)

Returns start of tomorrow (00:00:00 UTC) as timestamp

**Examples:**

```aql
TOMORROW() // Start of tomorrow
deadline < TOMORROW() // Due today
```

*Source: `date_functions.h`*

---

### UNIX_TIMESTAMP

**Signature:** `UNIX_TIMESTAMP()` → `integer`    
**Non-deterministic** (result may vary)

MySQL compatible: Returns current Unix timestamp in seconds

**Examples:**

```aql
UNIX_TIMESTAMP() // 1700000000
```

*Source: `date_functions.h`*

---

### WEEKS

**Signature:** `WEEKS(amount)` → `integer`  

Creates an interval of N weeks in milliseconds

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `amount` | `number` | ✅ | Number of weeks (can be fractional or negative) |

**Examples:**

```aql
WEEKS(2) // 1209600000 ms
DATE_NOW() + WEEKS(4)
WEEKS(-1) // One week ago
```

*Source: `date_functions.h`*

---

### WORKDAYS

**Signature:** `WORKDAYS(startDate, endDate, holidays?)` → `integer`  

Counts business days (Mon-Fri) between two dates, optionally excluding holidays

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `startDate` | `integer` | ✅ | Start timestamp in ms |
| `endDate` | `integer` | ✅ | End timestamp in ms |
| `holidays` | `array` | — | Optional array of holiday timestamps |

**Examples:**

```aql
WORKDAYS(DATE_TIMESTAMP("2024-01-01"), DATE_TIMESTAMP("2024-01-31"))
WORKDAYS(startDate, endDate, [DATE_TIMESTAMP("2024-12-25")])
```

*Source: `date_functions.h`*

---

### WORKDAYS_ADD

**Signature:** `WORKDAYS_ADD(startDate, workdays, holidays?)` → `integer`  

Adds N business days to a date, skipping weekends and holidays

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `startDate` | `integer` | ✅ | Start timestamp in ms |
| `workdays` | `integer` | ✅ | Number of workdays to add (can be negative) |
| `holidays` | `array` | — | Optional array of holiday timestamps |

**Examples:**

```aql
WORKDAYS_ADD(DATE_NOW(), 10) // 10 business days from now
WORKDAYS_ADD(startDate, -5) // 5 business days ago
```

*Source: `date_functions.h`*

---

### YEARS

**Signature:** `YEARS(amount)` → `integer`  

Creates an interval of N years in milliseconds

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `amount` | `number` | ✅ | Number of years (can be fractional or negative) |

**Examples:**

```aql
YEARS(1) // 31557600000 ms (365.25 days)
DATE_NOW() + YEARS(2)
YEARS(-1) // One year ago
```

*Source: `date_functions.h`*

---

### YESTERDAY

**Signature:** `YESTERDAY()` → `integer`    
**Non-deterministic** (result may vary)

Returns start of yesterday (00:00:00 UTC) as timestamp

**Examples:**

```aql
YESTERDAY() // Start of yesterday
event.date >= YESTERDAY() AND event.date < TODAY()
```

*Source: `date_functions.h`*

---

## Document Functions

| Function | Description |
|----------|-------------|
| [ATTRIBUTES](#attributes) | Returns an array of all keys in an object |
| [DOCUMENT](#document) | Loads a document from a collection by key |
| [HAS](#has) | Checks if an object has a specific key |
| [KEEP](#keep) | Returns a copy of the object with only specified keys |
| [MERGE](#merge) | Merges multiple objects into one (later values override) |
| [MERGE_RECURSIVE](#merge_recursive) | Deep merges multiple objects (nested objects are merged) |
| [UNSET](#unset) | Returns a copy of the object with specified keys removed |
| [UNZIP](#unzip) | Splits an object into arrays of keys and values |
| [VALUES](#values) | Returns an array of all values in an object |
| [ZIP](#zip) | Creates an object from arrays of keys and values |

### ATTRIBUTES

**Signature:** `ATTRIBUTES(obj)` → `array`  

Returns an array of all keys in an object

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `obj` | `object` | ✅ | Input object |

**Examples:**

```aql
ATTRIBUTES({a: 1, b: 2}) // ["a", "b"]
```

*Source: `document_functions.h`*

---

### DOCUMENT

**Signature:** `DOCUMENT(collection, key)` → `object`    
**Non-deterministic** (result may vary)

Loads a document from a collection by key

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `collection` | `string` | ✅ | Collection name |
| `key` | `string` | ✅ | Document key |

**Examples:**

```aql
DOCUMENT("users", "user123")
```

*Source: `document_functions.h`*

---

### HAS

**Signature:** `HAS(obj, key)` → `boolean`  

Checks if an object has a specific key

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `obj` | `object` | ✅ | Input object |
| `key` | `string` | ✅ | Key to check |

**Examples:**

```aql
HAS({a: 1, b: 2}, "a") // true
```

*Source: `document_functions.h`*

---

### KEEP

**Signature:** `KEEP(obj, keys)` → `object`  

Returns a copy of the object with only specified keys

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `obj` | `object` | ✅ | Input object |
| `keys` | `array` | ✅ | Array of keys to keep |

**Examples:**

```aql
KEEP({a: 1, b: 2, c: 3}, ["a", "c"]) // {a: 1, c: 3}
```

*Source: `document_functions.h`*

---

### MERGE

**Signature:** `MERGE(objects)` → `object`  

Merges multiple objects into one (later values override)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `objects` | `object` | ✅ | Objects to merge (variadic) |

**Examples:**

```aql
MERGE({a: 1}, {b: 2}) // {a: 1, b: 2}
```

*Source: `document_functions.h`*

---

### MERGE_RECURSIVE

**Signature:** `MERGE_RECURSIVE(objects)` → `object`  

Deep merges multiple objects (nested objects are merged)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `objects` | `object` | ✅ | Objects to merge (variadic) |

**Examples:**

```aql
MERGE_RECURSIVE({a: {x: 1}}, {a: {y: 2}}) // {a: {x: 1, y: 2}}
```

*Source: `document_functions.h`*

---

### UNSET

**Signature:** `UNSET(obj, keys)` → `object`  

Returns a copy of the object with specified keys removed

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `obj` | `object` | ✅ | Input object |
| `keys` | `array` | ✅ | Array of keys to remove |

**Examples:**

```aql
UNSET({a: 1, b: 2, c: 3}, ["b"]) // {a: 1, c: 3}
```

*Source: `document_functions.h`*

---

### UNZIP

**Signature:** `UNZIP(obj)` → `object`  

Splits an object into arrays of keys and values

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `obj` | `object` | ✅ | Input object |

**Examples:**

```aql
UNZIP({a: 1, b: 2}) // {keys: ["a", "b"], values: [1, 2]}
```

*Source: `document_functions.h`*

---

### VALUES

**Signature:** `VALUES(obj)` → `array`  

Returns an array of all values in an object

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `obj` | `object` | ✅ | Input object |

**Examples:**

```aql
VALUES({a: 1, b: 2}) // [1, 2]
```

*Source: `document_functions.h`*

---

### ZIP

**Signature:** `ZIP(keys, values)` → `object`  

Creates an object from arrays of keys and values

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `keys` | `array` | ✅ | Array of keys |
| `values` | `array` | ✅ | Array of values |

**Examples:**

```aql
ZIP(["a", "b"], [1, 2]) // {a: 1, b: 2}
```

*Source: `document_functions.h`*

---

## JSON Functions

| Function | Description |
|----------|-------------|
| [JSON_CONTAINS](#json_contains) | Checks if a JSON document contains a specific value |
| [JSON_DEPTH](#json_depth) | Returns the maximum depth of a JSON structure |
| [JSON_EXTRACT](#json_extract) | Extracts a value from a JSON document using a JSONPath expression |
| [JSON_PARSE](#json_parse) | Parses a JSON string into a JSON object |
| [JSON_REMOVE](#json_remove) | Removes a value from a JSON document at the specified path |
| [JSON_SET](#json_set) | Sets a value in a JSON document at the specified path |
| [JSON_STRINGIFY](#json_stringify) | Converts a value to a JSON string |
| [JSON_TYPE](#json_type) | Returns the type of the value at the specified path |

### JSON_CONTAINS

**Signature:** `JSON_CONTAINS(document, value)` → `boolean`  

Checks if a JSON document contains a specific value

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `document` | `object` | ✅ | JSON document |
| `value` | `any` | ✅ | Value to search for |

**Examples:**

```aql
JSON_CONTAINS({a: 1, b: {c: 2}}, 2) // true
JSON_CONTAINS([1, 2, 3], 4) // false
```

*Source: `json_path_functions.h`*

---

### JSON_DEPTH

**Signature:** `JSON_DEPTH(document)` → `number`  

Returns the maximum depth of a JSON structure

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `document` | `any` | ✅ | JSON document |

**Examples:**

```aql
JSON_DEPTH({a: {b: {c: 1}}}) // 3
JSON_DEPTH([1, [2, [3]]]) // 3
```

*Source: `json_path_functions.h`*

---

### JSON_EXTRACT

**Signature:** `JSON_EXTRACT(document, path)` → `any`  

Extracts a value from a JSON document using a JSONPath expression

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `document` | `object` | ✅ | JSON document |
| `path` | `string` | ✅ | JSONPath expression (e.g., '$.field.nested[0]') |

**Examples:**

```aql
JSON_EXTRACT({a: {b: 1}}, "$.a.b") // 1
JSON_EXTRACT({arr: [1,2,3]}, "$.arr[0]") // 1
```

*Source: `json_path_functions.h`*

---

### JSON_PARSE

**Signature:** `JSON_PARSE(json_string)` → `any`  

Parses a JSON string into a JSON object

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `json_string` | `string` | ✅ | JSON string to parse |

**Examples:**

```aql
JSON_PARSE('{"a": 1, "b": 2}') // {a: 1, b: 2}
JSON_PARSE('[1, 2, 3]') // [1, 2, 3]
```

*Source: `json_path_functions.h`*

---

### JSON_REMOVE

**Signature:** `JSON_REMOVE(document, path)` → `object`  

Removes a value from a JSON document at the specified path

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `document` | `object` | ✅ | JSON document |
| `path` | `string` | ✅ | JSONPath expression |

**Examples:**

```aql
JSON_REMOVE({a: 1, b: 2}, "$.b") // {a: 1}
JSON_REMOVE({arr: [1,2,3]}, "$.arr[1]") // {arr: [1,3]}
```

*Source: `json_path_functions.h`*

---

### JSON_SET

**Signature:** `JSON_SET(document, path, value)` → `object`  

Sets a value in a JSON document at the specified path

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `document` | `object` | ✅ | JSON document |
| `path` | `string` | ✅ | JSONPath expression |
| `value` | `any` | ✅ | Value to set |

**Examples:**

```aql
JSON_SET({a: 1}, "$.b", 2) // {a: 1, b: 2}
JSON_SET({arr: []}, "$.arr[0]", "hello") // {arr: ["hello"]}
```

*Source: `json_path_functions.h`*

---

### JSON_STRINGIFY

**Signature:** `JSON_STRINGIFY(value)` → `string`  

Converts a value to a JSON string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to stringify |

**Examples:**

```aql
JSON_STRINGIFY({a: 1, b: 2}) // '{"a":1,"b":2}'
JSON_STRINGIFY([1, 2, 3]) // '[1,2,3]'
```

*Source: `json_path_functions.h`*

---

### JSON_TYPE

**Signature:** `JSON_TYPE(document, path)` → `string`  

Returns the type of the value at the specified path

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `document` | `object` | ✅ | JSON document |
| `path` | `string` | ✅ | JSONPath expression |

**Examples:**

```aql
JSON_TYPE({a: 123}, "$.a") // "number"
JSON_TYPE({arr: []}, "$.arr") // "array"
```

*Source: `json_path_functions.h`*

---

## Geo Functions

| Function | Description |
|----------|-------------|
| [CRS_IS_GEOGRAPHIC](#crs_is_geographic) | Check if a CRS uses geographic (lat/lon) coordinates |
| [CRS_IS_PROJECTED](#crs_is_projected) | Check if a CRS uses projected (meter) coordinates |
| [CRS_NAME](#crs_name) | Get the name of a coordinate reference system by EPSG code |
| [GEO_CONTAINS](#geo_contains) | Test if polygon contains point (ArangoDB compatible) |
| [GEO_DISTANCE](#geo_distance) | Calculate great-circle distance in meters (ArangoDB compatible) |
| [ST_AREA](#st_area) | Calculate area of a Polygon (square units) |
| [ST_ASGEOJSON](#st_asgeojson) | Convert geometry to GeoJSON string |
| [ST_ASTEXT](#st_astext) | Convert geometry to Well-Known Text (WKT) string |
| [ST_CENTROID](#st_centroid) | Calculate centroid (center of mass) of geometry |
| [ST_CONTAINS](#st_contains) | Test if first geometry completely contains second geometry |
| [ST_DISTANCE](#st_distance) | Calculate distance between two geometries (meters for geographic, units for projected) |
| [ST_DWITHIN](#st_dwithin) | Test if two geometries are within specified distance |
| [ST_ENVELOPE](#st_envelope) | Calculate minimum bounding rectangle as Polygon |
| [ST_GEOMFROMGEOJSON](#st_geomfromgeojson) | Parse GeoJSON string or return GeoJSON object as-is |
| [ST_GEOMFROMTEXT](#st_geomfromtext) | Parse Well-Known Text (WKT) to GeoJSON geometry |
| [ST_HASZ](#st_hasz) | Check if geometry has Z coordinates |
| [ST_INTERSECTS](#st_intersects) | Test if two geometries spatially intersect |
| [ST_LENGTH](#st_length) | Calculate length of a LineString (meters for geographic) |
| [ST_LINESTRING](#st_linestring) | Create a LineString geometry from array of coordinate pairs |
| [ST_MAKEPOINT_UTM](#st_makepoint_utm) | Create a WGS84 Point from UTM coordinates |
| [ST_POINT](#st_point) | Create a Point geometry from coordinates |
| [ST_POLYGON](#st_polygon) | Create a Polygon geometry from array of rings (first is exterior, rest are holes) |
| [ST_SETSRID](#st_setsrid) | Set the SRID of a geometry without transforming coordinates |
| [ST_SRID](#st_srid) | Get or set the Spatial Reference ID (EPSG code) of a geometry |
| [ST_TRANSFORM](#st_transform) | Transform geometry from one coordinate reference system to another |
| [ST_WITHIN](#st_within) | Test if first geometry is completely within second geometry |
| [ST_X](#st_x) | Extract X coordinate (longitude) from Point |
| [ST_Y](#st_y) | Extract Y coordinate (latitude) from Point |
| [ST_Z](#st_z) | Extract Z coordinate (elevation) from Point, or null if 2D |
| [UTM_EPSG](#utm_epsg) | Get EPSG code for a UTM zone |
| [UTM_ZONE](#utm_zone) | Calculate UTM zone number from longitude |

### CRS_IS_GEOGRAPHIC

**Signature:** `CRS_IS_GEOGRAPHIC(epsg)` → `boolean`  

Check if a CRS uses geographic (lat/lon) coordinates

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `epsg` | `integer` | ✅ | EPSG code |

**Examples:**

```aql
CRS_IS_GEOGRAPHIC(4326)  // Returns true
```

*Source: `crs_functions.h`*

---

### CRS_IS_PROJECTED

**Signature:** `CRS_IS_PROJECTED(epsg)` → `boolean`  

Check if a CRS uses projected (meter) coordinates

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `epsg` | `integer` | ✅ | EPSG code |

**Examples:**

```aql
CRS_IS_PROJECTED(25832)  // Returns true
```

*Source: `crs_functions.h`*

---

### CRS_NAME

**Signature:** `CRS_NAME(epsg)` → `string`  

Get the name of a coordinate reference system by EPSG code

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `epsg` | `integer` | ✅ | EPSG code |

**Examples:**

```aql
CRS_NAME(25832)  // Returns 'ETRS89 / UTM zone 32N'
```

*Source: `crs_functions.h`*

---

### GEO_CONTAINS

**Signature:** `GEO_CONTAINS(polygon, point)` → `boolean`  

Test if polygon contains point (ArangoDB compatible)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `polygon` | `geometry` | ✅ | Polygon geometry |
| `point` | `geometry` | ✅ | Point geometry |

**Examples:**

```aql
GEO_CONTAINS(polygon, point)
```

*Source: `geo_functions.h`*

---

### GEO_DISTANCE

**Signature:** `GEO_DISTANCE(geom1, geom2)` → `number`  

Calculate great-circle distance in meters (ArangoDB compatible)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geom1` | `geometry` | ✅ | First geometry |
| `geom2` | `geometry` | ✅ | Second geometry |

**Examples:**

```aql
GEO_DISTANCE(point1, point2)
```

*Source: `geo_functions.h`*

---

### ST_AREA

**Signature:** `ST_AREA(geometry)` → `number`  

Calculate area of a Polygon (square units)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | Polygon geometry |

**Examples:**

```aql
ST_AREA(polygon)
```

*Source: `geo_functions.h`*

---

### ST_ASGEOJSON

**Signature:** `ST_ASGEOJSON(geometry)` → `string`  

Convert geometry to GeoJSON string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | Any geometry |

**Examples:**

```aql
ST_ASGEOJSON(point)
```

*Source: `geo_functions.h`*

---

### ST_ASTEXT

**Signature:** `ST_ASTEXT(geometry)` → `string`  

Convert geometry to Well-Known Text (WKT) string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | Any geometry |

**Examples:**

```aql
ST_ASTEXT(point)
```

*Source: `geo_functions.h`*

---

### ST_CENTROID

**Signature:** `ST_CENTROID(geometry)` → `geometry`  

Calculate centroid (center of mass) of geometry

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | Any geometry |

**Examples:**

```aql
ST_CENTROID(polygon)
```

*Source: `geo_functions.h`*

---

### ST_CONTAINS

**Signature:** `ST_CONTAINS(geom1, geom2)` → `boolean`  

Test if first geometry completely contains second geometry

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geom1` | `geometry` | ✅ | Container geometry |
| `geom2` | `geometry` | ✅ | Contained geometry |

**Examples:**

```aql
ST_CONTAINS(polygon, point)
```

*Source: `geo_functions.h`*

---

### ST_DISTANCE

**Signature:** `ST_DISTANCE(geom1, geom2)` → `number`  

Calculate distance between two geometries (meters for geographic, units for projected)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geom1` | `geometry` | ✅ | First geometry |
| `geom2` | `geometry` | ✅ | Second geometry |

**Examples:**

```aql
ST_DISTANCE(ST_POINT(0,0), ST_POINT(1,1))
```

*Source: `geo_functions.h`*

---

### ST_DWITHIN

**Signature:** `ST_DWITHIN(geom1, geom2, distance)` → `boolean`  

Test if two geometries are within specified distance

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geom1` | `geometry` | ✅ | First geometry |
| `geom2` | `geometry` | ✅ | Second geometry |
| `distance` | `number` | ✅ | Maximum distance |

**Examples:**

```aql
ST_DWITHIN(point1, point2, 1000)
```

*Source: `geo_functions.h`*

---

### ST_ENVELOPE

**Signature:** `ST_ENVELOPE(geometry)` → `geometry`  

Calculate minimum bounding rectangle as Polygon

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | Any geometry |

**Examples:**

```aql
ST_ENVELOPE(linestring)
```

*Source: `geo_functions.h`*

---

### ST_GEOMFROMGEOJSON

**Signature:** `ST_GEOMFROMGEOJSON(geojson)` → `geometry`  

Parse GeoJSON string or return GeoJSON object as-is

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geojson` | `any` | ✅ | GeoJSON string or object |

**Examples:**

```aql
ST_GEOMFROMGEOJSON('{\"type\":\"Point\",\"coordinates\":[1,2]}')
```

*Source: `geo_functions.h`*

---

### ST_GEOMFROMTEXT

**Signature:** `ST_GEOMFROMTEXT(wkt)` → `geometry`  

Parse Well-Known Text (WKT) to GeoJSON geometry

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `wkt` | `string` | ✅ | WKT string (e.g., 'POINT(1 2)') |

**Examples:**

```aql
ST_GEOMFROMTEXT('POINT(1 2)')
ST_GEOMFROMTEXT('LINESTRING(0 0, 1 1)')
```

*Source: `geo_functions.h`*

---

### ST_HASZ

**Signature:** `ST_HASZ(geometry)` → `boolean`  

Check if geometry has Z coordinates

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | Any geometry |

**Examples:**

```aql
ST_HASZ(point3d)
```

*Source: `geo_functions.h`*

---

### ST_INTERSECTS

**Signature:** `ST_INTERSECTS(geom1, geom2)` → `boolean`  

Test if two geometries spatially intersect

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geom1` | `geometry` | ✅ | First geometry |
| `geom2` | `geometry` | ✅ | Second geometry |

**Examples:**

```aql
ST_INTERSECTS(polygon, point)
```

*Source: `geo_functions.h`*

---

### ST_LENGTH

**Signature:** `ST_LENGTH(geometry)` → `number`  

Calculate length of a LineString (meters for geographic)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | LineString geometry |

**Examples:**

```aql
ST_LENGTH(linestring)
```

*Source: `geo_functions.h`*

---

### ST_LINESTRING

**Signature:** `ST_LINESTRING(coordinates)` → `geometry`  

Create a LineString geometry from array of coordinate pairs

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `coordinates` | `array` | ✅ | Array of [x,y] or [x,y,z] coordinates |

**Examples:**

```aql
ST_LINESTRING([[0,0], [1,1], [2,0]])
```

*Source: `geo_functions.h`*

---

### ST_MAKEPOINT_UTM

**Signature:** `ST_MAKEPOINT_UTM(easting, northing, zone, hemisphere?)` → `geometry`  

Create a WGS84 Point from UTM coordinates

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `easting` | `number` | ✅ | Easting in meters |
| `northing` | `number` | ✅ | Northing in meters |
| `zone` | `integer` | ✅ | UTM zone (1-60) |
| `hemisphere` | `string` | — | 'N' or 'S' |

**Examples:**

```aql
ST_MAKEPOINT_UTM(500000, 5500000, 32, 'N')  // Create point from UTM32N
```

*Source: `crs_functions.h`*

---

### ST_POINT

**Signature:** `ST_POINT(x, y, z?)` → `geometry`  

Create a Point geometry from coordinates

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `x` | `number` | ✅ | X coordinate (longitude) |
| `y` | `number` | ✅ | Y coordinate (latitude) |
| `z` | `number` | — | Z coordinate (elevation) |

**Examples:**

```aql
ST_POINT(13.4, 52.5)
ST_POINT(13.4, 52.5, 100)
```

*Source: `geo_functions.h`*

---

### ST_POLYGON

**Signature:** `ST_POLYGON(rings)` → `geometry`  

Create a Polygon geometry from array of rings (first is exterior, rest are holes)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `rings` | `array` | ✅ | Array of linear rings |

**Examples:**

```aql
ST_POLYGON([[[0,0], [1,0], [1,1], [0,1], [0,0]]])
```

*Source: `geo_functions.h`*

---

### ST_SETSRID

**Signature:** `ST_SETSRID(geometry, srid)` → `geometry`  

Set the SRID of a geometry without transforming coordinates

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | Input geometry |
| `srid` | `integer` | ✅ | EPSG code to assign |

**Examples:**

```aql
ST_SETSRID(point, 25832)  // Mark point as UTM32N
```

*Source: `crs_functions.h`*

---

### ST_SRID

**Signature:** `ST_SRID(geometry, srid?)` → `any`  

Get or set the Spatial Reference ID (EPSG code) of a geometry

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | Input geometry |
| `srid` | `integer` | — | New SRID to set (optional) |

**Examples:**

```aql
ST_SRID(point)  // Get SRID
ST_SRID(point, 4326)  // Set SRID
```

*Source: `crs_functions.h`*

---

### ST_TRANSFORM

**Signature:** `ST_TRANSFORM(geometry, from_srid, to_srid)` → `geometry`  

Transform geometry from one coordinate reference system to another

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geometry` | `geometry` | ✅ | Input geometry |
| `from_srid` | `integer` | ✅ | Source EPSG code (e.g., 25832) |
| `to_srid` | `integer` | ✅ | Target EPSG code (e.g., 4326) |

**Examples:**

```aql
ST_TRANSFORM(utm_point, 25832, 4326)  // ETRS89/UTM32N → WGS84
ST_TRANSFORM(wgs84_point, 4326, 25832)  // WGS84 → ETRS89/UTM32N
```

*Source: `crs_functions.h`*

---

### ST_WITHIN

**Signature:** `ST_WITHIN(geom1, geom2)` → `boolean`  

Test if first geometry is completely within second geometry

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `geom1` | `geometry` | ✅ | Inner geometry |
| `geom2` | `geometry` | ✅ | Outer geometry |

**Examples:**

```aql
ST_WITHIN(point, polygon)
```

*Source: `geo_functions.h`*

---

### ST_X

**Signature:** `ST_X(point)` → `number`  

Extract X coordinate (longitude) from Point

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `point` | `geometry` | ✅ | Point geometry |

**Examples:**

```aql
ST_X(point)
```

*Source: `geo_functions.h`*

---

### ST_Y

**Signature:** `ST_Y(point)` → `number`  

Extract Y coordinate (latitude) from Point

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `point` | `geometry` | ✅ | Point geometry |

**Examples:**

```aql
ST_Y(point)
```

*Source: `geo_functions.h`*

---

### ST_Z

**Signature:** `ST_Z(point)` → `nullable`  

Extract Z coordinate (elevation) from Point, or null if 2D

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `point` | `geometry` | ✅ | Point geometry |

**Examples:**

```aql
ST_Z(point3d)
```

*Source: `geo_functions.h`*

---

### UTM_EPSG

**Signature:** `UTM_EPSG(zone, hemisphere?, ellipsoid?)` → `integer`  

Get EPSG code for a UTM zone

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `zone` | `integer` | ✅ | UTM zone number (1-60) |
| `hemisphere` | `string` | — | 'N' for north or 'S' for south |
| `ellipsoid` | `string` | — | 'WGS84' or 'ETRS89' |

**Examples:**

```aql
UTM_EPSG(32)  // Returns 32632 (WGS84/UTM32N)
```

*Source: `crs_functions.h`*

---

### UTM_ZONE

**Signature:** `UTM_ZONE(longitude)` → `integer`  

Calculate UTM zone number from longitude

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `longitude` | `number` | ✅ | Longitude in degrees |

**Examples:**

```aql
UTM_ZONE(9.0)  // Returns 32 (Central Germany)
```

*Source: `crs_functions.h`*

---

## Vector Functions

| Function | Description |
|----------|-------------|
| [CHEBYSHEV_DISTANCE](#chebyshev_distance) | Calculate Chebyshev (L-infinity/max) distance between two vectors |
| [COSINE_SIMILARITY](#cosine_similarity) | Calculate cosine similarity between two vectors (1 = identical, -1 = opposite) |
| [DOT_PRODUCT](#dot_product) | Calculate dot product (inner product) of two vectors |
| [EUCLIDEAN_DISTANCE](#euclidean_distance) | Calculate Euclidean (L2) distance between two vectors |
| [L2_NORMALIZE](#l2_normalize) | Normalize vector to unit length (L2 norm = 1) |
| [MANHATTAN_DISTANCE](#manhattan_distance) | Calculate Manhattan (L1/taxicab) distance between two vectors |
| [MIN_MAX_NORMALIZE](#min_max_normalize) | Scale vector values to [0,1] range based on min/max |
| [SIMILARITY](#similarity) | Calculate similarity score (higher = more similar). Used in vector search queries. |
| [VECTOR_ADD](#vector_add) | Add two vectors element-wise |
| [VECTOR_AVG](#vector_avg) | Calculate average (mean) of all vector elements |
| [VECTOR_CONCAT](#vector_concat) | Concatenate multiple vectors into one |
| [VECTOR_DIM](#vector_dim) | Get dimensionality (number of elements) of vector |
| [VECTOR_MAX](#vector_max) | Get maximum element in vector |
| [VECTOR_MIN](#vector_min) | Get minimum element in vector |
| [VECTOR_MUL](#vector_mul) | Multiply two vectors element-wise (Hadamard product) |
| [VECTOR_NORM](#vector_norm) | Calculate Lp norm of vector (default L2/Euclidean) |
| [VECTOR_ONES](#vector_ones) | Create a vector of ones with specified dimension |
| [VECTOR_RANDOM](#vector_random) | Create a random vector with values in [min, max] range |
| [VECTOR_SCALE](#vector_scale) | Multiply all vector elements by a scalar |
| [VECTOR_SLICE](#vector_slice) | Extract a sub-vector from start to end index |
| [VECTOR_SUB](#vector_sub) | Subtract second vector from first element-wise |
| [VECTOR_SUM](#vector_sum) | Calculate sum of all vector elements |
| [VECTOR_ZEROS](#vector_zeros) | Create a zero vector of specified dimension |

### CHEBYSHEV_DISTANCE

**Signature:** `CHEBYSHEV_DISTANCE(vec1, vec2)` → `number`  

Calculate Chebyshev (L-infinity/max) distance between two vectors

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec1` | `vector` | ✅ | First vector |
| `vec2` | `vector` | ✅ | Second vector |

**Examples:**

```aql
CHEBYSHEV_DISTANCE([0,0], [3,4]) = 4
```

*Source: `vector_functions.h`*

---

### COSINE_SIMILARITY

**Signature:** `COSINE_SIMILARITY(vec1, vec2)` → `number`  

Calculate cosine similarity between two vectors (1 = identical, -1 = opposite)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec1` | `vector` | ✅ | First vector |
| `vec2` | `vector` | ✅ | Second vector |

**Examples:**

```aql
COSINE_SIMILARITY([1,0,0], [1,0,0]) = 1.0
COSINE_SIMILARITY([1,0], [0,1]) = 0.0
```

*Source: `vector_functions.h`*

---

### DOT_PRODUCT

**Signature:** `DOT_PRODUCT(vec1, vec2)` → `number`  

Calculate dot product (inner product) of two vectors

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec1` | `vector` | ✅ | First vector |
| `vec2` | `vector` | ✅ | Second vector |

**Examples:**

```aql
DOT_PRODUCT([1,2,3], [4,5,6]) = 32
```

*Source: `vector_functions.h`*

---

### EUCLIDEAN_DISTANCE

**Signature:** `EUCLIDEAN_DISTANCE(vec1, vec2)` → `number`  

Calculate Euclidean (L2) distance between two vectors

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec1` | `vector` | ✅ | First vector |
| `vec2` | `vector` | ✅ | Second vector |

**Examples:**

```aql
EUCLIDEAN_DISTANCE([0,0], [3,4]) = 5.0
```

*Source: `vector_functions.h`*

---

### L2_NORMALIZE

**Signature:** `L2_NORMALIZE(vec)` → `vector`  

Normalize vector to unit length (L2 norm = 1)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Vector to normalize |

**Examples:**

```aql
L2_NORMALIZE([3,4]) = [0.6, 0.8]
```

*Source: `vector_functions.h`*

---

### MANHATTAN_DISTANCE

**Signature:** `MANHATTAN_DISTANCE(vec1, vec2)` → `number`  

Calculate Manhattan (L1/taxicab) distance between two vectors

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec1` | `vector` | ✅ | First vector |
| `vec2` | `vector` | ✅ | Second vector |

**Examples:**

```aql
MANHATTAN_DISTANCE([0,0], [3,4]) = 7
```

*Source: `vector_functions.h`*

---

### MIN_MAX_NORMALIZE

**Signature:** `MIN_MAX_NORMALIZE(vec, min?, max?)` → `vector`  

Scale vector values to [0,1] range based on min/max

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Vector to normalize |
| `min` | `number` | — | Minimum value (auto-detected if not provided) |
| `max` | `number` | — | Maximum value (auto-detected if not provided) |

**Examples:**

```aql
MIN_MAX_NORMALIZE([2,4,6], 0, 10) = [0.2, 0.4, 0.6]
```

*Source: `vector_functions.h`*

---

### SIMILARITY

**Signature:** `SIMILARITY(vec1, vec2, k?)` → `number`  

Calculate similarity score (higher = more similar). Used in vector search queries.

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec1` | `vector` | ✅ | Query vector |
| `vec2` | `vector` | ✅ | Target vector |
| `k` | `integer` | — | Number of results (for index search) |

**Examples:**

```aql
SIMILARITY(query_embedding, doc._embedding, 5)
```

*Source: `vector_functions.h`*

---

### VECTOR_ADD

**Signature:** `VECTOR_ADD(vec1, vec2)` → `vector`  

Add two vectors element-wise

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec1` | `vector` | ✅ | First vector |
| `vec2` | `vector` | ✅ | Second vector |

**Examples:**

```aql
VECTOR_ADD([1,2,3], [4,5,6]) = [5,7,9]
```

*Source: `vector_functions.h`*

---

### VECTOR_AVG

**Signature:** `VECTOR_AVG(vec)` → `number`  

Calculate average (mean) of all vector elements

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Input vector |

**Examples:**

```aql
VECTOR_AVG([1,2,3,4]) = 2.5
```

*Source: `vector_functions.h`*

---

### VECTOR_CONCAT

**Signature:** `VECTOR_CONCAT(vectors)` → `vector`  

Concatenate multiple vectors into one

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vectors` | `any` | ✅ | Vectors to concatenate (variadic) |

**Examples:**

```aql
VECTOR_CONCAT([1,2], [3,4]) = [1,2,3,4]
```

*Source: `vector_functions.h`*

---

### VECTOR_DIM

**Signature:** `VECTOR_DIM(vec)` → `integer`  

Get dimensionality (number of elements) of vector

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Input vector |

**Examples:**

```aql
VECTOR_DIM([1,2,3]) = 3
```

*Source: `vector_functions.h`*

---

### VECTOR_MAX

**Signature:** `VECTOR_MAX(vec)` → `number`  

Get maximum element in vector

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Input vector |

**Examples:**

```aql
VECTOR_MAX([3,1,4,1,5]) = 5
```

*Source: `vector_functions.h`*

---

### VECTOR_MIN

**Signature:** `VECTOR_MIN(vec)` → `number`  

Get minimum element in vector

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Input vector |

**Examples:**

```aql
VECTOR_MIN([3,1,4,1,5]) = 1
```

*Source: `vector_functions.h`*

---

### VECTOR_MUL

**Signature:** `VECTOR_MUL(vec1, vec2)` → `vector`  

Multiply two vectors element-wise (Hadamard product)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec1` | `vector` | ✅ | First vector |
| `vec2` | `vector` | ✅ | Second vector |

**Examples:**

```aql
VECTOR_MUL([1,2,3], [4,5,6]) = [4,10,18]
```

*Source: `vector_functions.h`*

---

### VECTOR_NORM

**Signature:** `VECTOR_NORM(vec, p?)` → `number`  

Calculate Lp norm of vector (default L2/Euclidean)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Input vector |
| `p` | `number` | — | Norm order (1=Manhattan, 2=Euclidean, inf=Chebyshev) |

**Examples:**

```aql
VECTOR_NORM([3,4]) = 5
VECTOR_NORM([3,4], 1) = 7
```

*Source: `vector_functions.h`*

---

### VECTOR_ONES

**Signature:** `VECTOR_ONES(dimension)` → `vector`  

Create a vector of ones with specified dimension

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `dimension` | `integer` | ✅ | Number of dimensions |

**Examples:**

```aql
VECTOR_ONES(3) = [1,1,1]
```

*Source: `vector_functions.h`*

---

### VECTOR_RANDOM

**Signature:** `VECTOR_RANDOM(dimension, min?, max?)` → `vector`    
**Non-deterministic** (result may vary)

Create a random vector with values in [min, max] range

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `dimension` | `integer` | ✅ | Number of dimensions |
| `min` | `number` | — | Minimum value |
| `max` | `number` | — | Maximum value |

**Examples:**

```aql
VECTOR_RANDOM(3)
VECTOR_RANDOM(5, -1, 1)
```

*Source: `vector_functions.h`*

---

### VECTOR_SCALE

**Signature:** `VECTOR_SCALE(vec, scalar)` → `vector`  

Multiply all vector elements by a scalar

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Vector to scale |
| `scalar` | `number` | ✅ | Scalar multiplier |

**Examples:**

```aql
VECTOR_SCALE([1,2,3], 2) = [2,4,6]
```

*Source: `vector_functions.h`*

---

### VECTOR_SLICE

**Signature:** `VECTOR_SLICE(vec, start, end?)` → `vector`  

Extract a sub-vector from start to end index

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Input vector |
| `start` | `integer` | ✅ | Start index (inclusive) |
| `end` | `integer` | — | End index (exclusive, default: vector length) |

**Examples:**

```aql
VECTOR_SLICE([1,2,3,4,5], 1, 4) = [2,3,4]
```

*Source: `vector_functions.h`*

---

### VECTOR_SUB

**Signature:** `VECTOR_SUB(vec1, vec2)` → `vector`  

Subtract second vector from first element-wise

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec1` | `vector` | ✅ | First vector |
| `vec2` | `vector` | ✅ | Second vector (subtracted) |

**Examples:**

```aql
VECTOR_SUB([5,7,9], [1,2,3]) = [4,5,6]
```

*Source: `vector_functions.h`*

---

### VECTOR_SUM

**Signature:** `VECTOR_SUM(vec)` → `number`  

Calculate sum of all vector elements

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vec` | `vector` | ✅ | Input vector |

**Examples:**

```aql
VECTOR_SUM([1,2,3,4]) = 10
```

*Source: `vector_functions.h`*

---

### VECTOR_ZEROS

**Signature:** `VECTOR_ZEROS(dimension)` → `vector`  

Create a zero vector of specified dimension

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `dimension` | `integer` | ✅ | Number of dimensions |

**Examples:**

```aql
VECTOR_ZEROS(3) = [0,0,0]
```

*Source: `vector_functions.h`*

---

## Graph Functions

| Function | Description |
|----------|-------------|
| [ALL_SHORTEST_PATHS](#all_shortest_paths) | Returns all shortest paths between two vertices |
| [BETWEENNESS_CENTRALITY](#betweenness_centrality) | Calculates betweenness centrality for all vertices |
| [CLOSENESS_CENTRALITY](#closeness_centrality) | Calculates closeness centrality for all vertices |
| [CLUSTERING_COEFFICIENT](#clustering_coefficient) | Calculate local clustering coefficient for a vertex |
| [CONNECTED_COMPONENTS](#connected_components) | Find all connected components (undirected) |
| [DEGREE_CENTRALITY](#degree_centrality) | Calculate degree centrality (normalized degree) |
| [EDGES](#edges) | Get all edges connected to a vertex |
| [GRAPH_CONNECTED](#graph_connected) | Check if two vertices are connected |
| [GRAPH_DEGREE](#graph_degree) | Calculate degree of a vertex (number of connected edges) |
| [GRAPH_DISTANCE](#graph_distance) | Calculate shortest path distance between two vertices |
| [GRAPH_NEIGHBORS](#graph_neighbors) | Get neighbors of a vertex up to specified depth |
| [IS_EDGE](#is_edge) | Check if document is an edge (has _from and _to fields) |
| [IS_VERTEX](#is_vertex) | Check if document is a vertex (has _id but not _from/_to) |
| [K_SHORTEST_PATHS](#k_shortest_paths) | Returns the K shortest paths between two vertices using Yen's algorithm |
| [LABEL_PROPAGATION_COMMUNITIES](#label_propagation_communities) | Fast community detection using label propagation (neighbors voting) |
| [LOUVAIN_COMMUNITIES](#louvain_communities) | Detect communities using Louvain algorithm (greedy modularity optimization) |
| [PAGERANK](#pagerank) | Calculate PageRank scores for all vertices with degree information |
| [PARSE_IDENTIFIER](#parse_identifier) | Parse document ID into {collection, key} object |
| [PATH_EDGES](#path_edges) | Extracts the edges from a path object |
| [PATH_LENGTH](#path_length) | Returns the number of edges in a path |
| [PATH_VERTICES](#path_vertices) | Extracts the vertices from a path object |
| [PATH_WEIGHT](#path_weight) | Calculates the total weight of edges in a path |
| [SHORTEST_PATH](#shortest_path) | Find shortest path between two vertices using BFS |
| [VERTICES](#vertices) | Extract vertex IDs from a path or array of paths |
| [WEIGHTED_SHORTEST_PATH](#weighted_shortest_path) | Finds the shortest weighted path using Dijkstra's algorithm |

### ALL_SHORTEST_PATHS

**Signature:** `ALL_SHORTEST_PATHS(startVertex, endVertex, options?)` → `array`  

Returns all shortest paths between two vertices

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `startVertex` | `string` | ✅ | Starting vertex ID |
| `endVertex` | `string` | ✅ | Ending vertex ID |
| `options` | `object` | — | Optional parameters |

**Examples:**

```aql
ALL_SHORTEST_PATHS("A", "B")
```

*Source: `graph_extensions.h`*

---

### BETWEENNESS_CENTRALITY

**Signature:** `BETWEENNESS_CENTRALITY(graphName)` → `object`  

Calculates betweenness centrality for all vertices

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `graphName` | `string` | ✅ | Graph name or edge collection |

**Examples:**

```aql
BETWEENNESS_CENTRALITY("myGraph")
```

*Source: `graph_extensions.h`*

---

### CLOSENESS_CENTRALITY

**Signature:** `CLOSENESS_CENTRALITY(graphName)` → `object`  

Calculates closeness centrality for all vertices

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `graphName` | `string` | ✅ | Graph name or edge collection |

**Examples:**

```aql
CLOSENESS_CENTRALITY("myGraph")
```

*Source: `graph_extensions.h`*

---

### CLUSTERING_COEFFICIENT

**Signature:** `CLUSTERING_COEFFICIENT(vertex, edges)` → `number`  

Calculate local clustering coefficient for a vertex

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vertex` | `any` | ✅ | Vertex ID or document |
| `edges` | `array` | ✅ | Array of edge documents |

**Examples:**

```aql
CLUSTERING_COEFFICIENT('users/1', edges)
```

*Source: `graph_functions.h`*

---

### CONNECTED_COMPONENTS

**Signature:** `CONNECTED_COMPONENTS(edges)` → `array`  

Find all connected components (undirected)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `edges` | `array` | ✅ | Array of edge documents |

**Examples:**

```aql
CONNECTED_COMPONENTS(edges)
```

*Source: `graph_functions.h`*

---

### DEGREE_CENTRALITY

**Signature:** `DEGREE_CENTRALITY(vertex, edges, direction?)` → `number`  

Calculate degree centrality (normalized degree)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vertex` | `any` | ✅ | Vertex ID or document |
| `edges` | `array` | ✅ | Array of edge documents |
| `direction` | `string` | — | Direction: 'outbound', 'inbound', or 'any' |

**Examples:**

```aql
DEGREE_CENTRALITY('users/1', edges)
```

*Source: `graph_functions.h`*

---

### EDGES

**Signature:** `EDGES(vertex, edges, direction?)` → `array`  

Get all edges connected to a vertex

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vertex` | `any` | ✅ | Vertex ID or document |
| `edges` | `array` | ✅ | Array of edge documents |
| `direction` | `string` | — | Direction: 'outbound', 'inbound', or 'any' |

**Examples:**

```aql
EDGES('users/1', edges, 'outbound')
```

*Source: `graph_functions.h`*

---

### GRAPH_CONNECTED

**Signature:** `GRAPH_CONNECTED(start, end, edges, direction?)` → `boolean`  

Check if two vertices are connected

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `start` | `any` | ✅ | Start vertex ID or document |
| `end` | `any` | ✅ | End vertex ID or document |
| `edges` | `array` | ✅ | Array of edge documents |
| `direction` | `string` | — | Direction: 'outbound', 'inbound', or 'any' |

**Examples:**

```aql
GRAPH_CONNECTED('a/1', 'b/2', edges)
```

*Source: `graph_functions.h`*

---

### GRAPH_DEGREE

**Signature:** `GRAPH_DEGREE(vertex, edges, direction?)` → `integer`  

Calculate degree of a vertex (number of connected edges)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vertex` | `any` | ✅ | Vertex ID or document |
| `edges` | `array` | ✅ | Array of edge documents |
| `direction` | `string` | — | Direction: 'outbound', 'inbound', or 'any' |

**Examples:**

```aql
GRAPH_DEGREE('users/1', edges, 'outbound')
```

*Source: `graph_functions.h`*

---

### GRAPH_DISTANCE

**Signature:** `GRAPH_DISTANCE(start, end, edges, direction?)` → `integer`  

Calculate shortest path distance between two vertices

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `start` | `any` | ✅ | Start vertex ID or document |
| `end` | `any` | ✅ | End vertex ID or document |
| `edges` | `array` | ✅ | Array of edge documents |
| `direction` | `string` | — | Direction: 'outbound', 'inbound', or 'any' |

**Examples:**

```aql
GRAPH_DISTANCE('a/1', 'b/2', edges)
```

*Source: `graph_functions.h`*

---

### GRAPH_NEIGHBORS

**Signature:** `GRAPH_NEIGHBORS(vertex, edges, direction?, depth?)` → `array`  

Get neighbors of a vertex up to specified depth

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `vertex` | `any` | ✅ | Start vertex ID or document |
| `edges` | `array` | ✅ | Array of edge documents |
| `direction` | `string` | — | Direction: 'outbound', 'inbound', or 'any' |
| `depth` | `integer` | — | Maximum traversal depth |

**Examples:**

```aql
GRAPH_NEIGHBORS('users/1', edges, 'outbound', 2)
```

*Source: `graph_functions.h`*

---

### IS_EDGE

**Signature:** `IS_EDGE(document)` → `boolean`  

Check if document is an edge (has _from and _to fields)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `document` | `object` | ✅ | Document to check |

**Examples:**

```aql
IS_EDGE({_from: 'a/1', _to: 'b/2'}) = true
```

*Source: `graph_functions.h`*

---

### IS_VERTEX

**Signature:** `IS_VERTEX(document)` → `boolean`  

Check if document is a vertex (has _id but not _from/_to)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `document` | `object` | ✅ | Document to check |

**Examples:**

```aql
IS_VERTEX({_id: 'users/1', name: 'Alice'}) = true
```

*Source: `graph_functions.h`*

---

### K_SHORTEST_PATHS

**Signature:** `K_SHORTEST_PATHS(startVertex, endVertex, k, options?)` → `array`  

Returns the K shortest paths between two vertices using Yen's algorithm

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `startVertex` | `string` | ✅ | Starting vertex ID |
| `endVertex` | `string` | ✅ | Ending vertex ID |
| `k` | `integer` | ✅ | Number of shortest paths to find |
| `options` | `object` | — | Optional parameters (weightAttribute, etc.) |

**Examples:**

```aql
K_SHORTEST_PATHS("A", "E", 3) // Find 3 shortest paths
K_SHORTEST_PATHS("A", "E", 5, {weightAttribute: "distance"})
```

*Source: `graph_extensions.h`*

---

### LABEL_PROPAGATION_COMMUNITIES

**Signature:** `LABEL_PROPAGATION_COMMUNITIES(edges, max_iterations?)` → `object`  

Fast community detection using label propagation (neighbors voting)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `edges` | `array` | ✅ | Array of edge documents |
| `max_iterations` | `integer` | — | Maximum number of propagation iterations (default: 100) |

**Examples:**

```aql
LABEL_PROPAGATION_COMMUNITIES(edges)
LABEL_PROPAGATION_COMMUNITIES(edges, 50)
```

*Source: `graph_functions.h`*

---

### LOUVAIN_COMMUNITIES

**Signature:** `LOUVAIN_COMMUNITIES(edges, min_modularity_gain?)` → `object`  

Detect communities using Louvain algorithm (greedy modularity optimization)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `edges` | `array` | ✅ | Array of edge documents |
| `min_modularity_gain` | `number` | — | Minimum modularity gain to continue optimization (default: 0.000001) |

**Examples:**

```aql
LOUVAIN_COMMUNITIES(edges)
LOUVAIN_COMMUNITIES(edges, 0.0001)
```

*Source: `graph_functions.h`*

---

### PAGERANK

**Signature:** `PAGERANK(edges, damping?, iterations?, options?)` → `array`  

Calculate PageRank scores for all vertices with degree information

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `edges` | `array` | ✅ | Array of edge documents |
| `damping` | `number` | — | Damping factor (default 0.85) |
| `iterations` | `integer` | — | Number of iterations |
| `options` | `object` | — | Options: {format: 'detailed'|'simple', epsilon: 1e-6}. 'detailed' returns ARRAY, 'simple' returns OBJECT |

**Examples:**

```aql
PAGERANK(edges)
PAGERANK(edges, 0.85, 100)
PAGERANK(edges, 0.85, 100, {format: 'simple'})
```

*Source: `graph_functions.h`*

---

### PARSE_IDENTIFIER

**Signature:** `PARSE_IDENTIFIER(id)` → `object`  

Parse document ID into {collection, key} object

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `id` | `string` | ✅ | Document ID (e.g., 'users/123') |

**Examples:**

```aql
PARSE_IDENTIFIER('users/123') = {collection: 'users', key: '123'}
```

*Source: `graph_functions.h`*

---

### PATH_EDGES

**Signature:** `PATH_EDGES(path)` → `array`  

Extracts the edges from a path object

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `object` | ✅ | Path object |

**Examples:**

```aql
PATH_EDGES(path)
```

*Source: `graph_extensions.h`*

---

### PATH_LENGTH

**Signature:** `PATH_LENGTH(path)` → `number`  

Returns the number of edges in a path

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `object` | ✅ | Path object |

**Examples:**

```aql
PATH_LENGTH(path)
```

*Source: `graph_extensions.h`*

---

### PATH_VERTICES

**Signature:** `PATH_VERTICES(path)` → `array`  

Extracts the vertices from a path object

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `object` | ✅ | Path object |

**Examples:**

```aql
PATH_VERTICES(path)
```

*Source: `graph_extensions.h`*

---

### PATH_WEIGHT

**Signature:** `PATH_WEIGHT(path, weightAttribute)` → `number`  

Calculates the total weight of edges in a path

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `object` | ✅ | Path object |
| `weightAttribute` | `string` | ✅ | Weight attribute name |

**Examples:**

```aql
PATH_WEIGHT(path, "distance")
```

*Source: `graph_extensions.h`*

---

### SHORTEST_PATH

**Signature:** `SHORTEST_PATH(start, end, edges, direction?)` → `object`  

Find shortest path between two vertices using BFS

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `start` | `any` | ✅ | Start vertex ID or document |
| `end` | `any` | ✅ | End vertex ID or document |
| `edges` | `array` | ✅ | Array of edge documents |
| `direction` | `string` | — | Direction: 'outbound', 'inbound', or 'any' |

**Examples:**

```aql
SHORTEST_PATH('a/1', 'b/2', edges, 'outbound')
```

*Source: `graph_functions.h`*

---

### VERTICES

**Signature:** `VERTICES(path)` → `array`  

Extract vertex IDs from a path or array of paths

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `any` | ✅ | Path object or array of paths |

**Examples:**

```aql
VERTICES(shortestPath)
```

*Source: `graph_functions.h`*

---

### WEIGHTED_SHORTEST_PATH

**Signature:** `WEIGHTED_SHORTEST_PATH(startVertex, endVertex, weightAttribute)` → `object`  

Finds the shortest weighted path using Dijkstra's algorithm

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `startVertex` | `string` | ✅ | Starting vertex ID |
| `endVertex` | `string` | ✅ | Ending vertex ID |
| `weightAttribute` | `string` | ✅ | Edge weight attribute name |

**Examples:**

```aql
WEIGHTED_SHORTEST_PATH("A", "B", "distance")
```

*Source: `graph_extensions.h`*

---

## Relational Functions

| Function | Description |
|----------|-------------|
| [COALESCE](#coalesce) | Return first non-null value from arguments |
| [COLLECT](#collect) | Collect/group array elements by a key field |
| [COUNT_DISTINCT](#count_distinct) | Count unique/distinct values in an array |
| [GREATEST](#greatest) | Return the greatest (maximum) value from arguments |
| [GROUP_CONCAT](#group_concat) | Concatenate array values into a string with separator |
| [IF](#if) | Return then_value if condition is true, else else_value |
| [INNER_JOIN](#inner_join) | Perform inner join on two arrays of objects |
| [LAG](#lag) | Add lagged (previous) values to array elements |
| [LEAD](#lead) | Add leading (next) values to array elements |
| [LEAST](#least) | Return the least (minimum) value from arguments |
| [LEFT_JOIN](#left_join) | Perform left outer join on two arrays of objects |
| [LOOKUP](#lookup) | Find first object in array where key equals value |
| [MEDIAN](#median) | Calculate median (50th percentile) |
| [NTILE](#ntile) | Divide array into n equal-sized buckets |
| [NULLIF](#nullif) | Return null if both values are equal, otherwise return first value |
| [PERCENTILE](#percentile) | Calculate percentile value (0-100) |
| [ROW_NUMBER](#row_number) | Add sequential row numbers to array elements |
| [RUNNING_SUM](#running_sum) | Calculate running (cumulative) sum |
| [STDDEV](#stddev) | Calculate population standard deviation |
| [STDDEV_SAMPLE](#stddev_sample) | Calculate sample standard deviation (n-1 denominator) |
| [VARIANCE](#variance) | Calculate population variance |

### COALESCE

**Signature:** `COALESCE(values)` → `any`  

Return first non-null value from arguments

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | ✅ | Values to check (variadic) |

**Examples:**

```aql
COALESCE(null, null, 'default')  // Returns 'default'
```

*Source: `relational_functions.h`*

---

### COLLECT

**Signature:** `COLLECT(array, key)` → `object`    
**Aggregate:** ✅

Collect/group array elements by a key field

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of objects |
| `key` | `string` | ✅ | Field name to group by |

**Examples:**

```aql
COLLECT([{type:'a',v:1},{type:'b',v:2},{type:'a',v:3}], 'type')
```

*Source: `relational_functions.h`*

---

### COUNT_DISTINCT

**Signature:** `COUNT_DISTINCT(array)` → `integer`    
**Aggregate:** ✅

Count unique/distinct values in an array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of values |

**Examples:**

```aql
COUNT_DISTINCT([1, 2, 2, 3, 3, 3])  // Returns 3
```

*Source: `relational_functions.h`*

---

### GREATEST

**Signature:** `GREATEST(values)` → `any`  

Return the greatest (maximum) value from arguments

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | ✅ | Values to compare (variadic) |

**Examples:**

```aql
GREATEST(1, 5, 3)  // Returns 5
```

*Source: `relational_functions.h`*

---

### GROUP_CONCAT

**Signature:** `GROUP_CONCAT(array, separator?)` → `string`    
**Aggregate:** ✅

Concatenate array values into a string with separator

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of values |
| `separator` | `string` | — | Separator string |

**Examples:**

```aql
GROUP_CONCAT(['a', 'b', 'c'], ', ')  // Returns 'a, b, c'
```

*Source: `relational_functions.h`*

---

### IF

**Signature:** `IF(condition, then_value, else_value)` → `any`  

Return then_value if condition is true, else else_value

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `condition` | `any` | ✅ | Boolean condition |
| `then_value` | `any` | ✅ | Value if true |
| `else_value` | `any` | ✅ | Value if false |

**Examples:**

```aql
IF(age >= 18, 'adult', 'minor')
```

*Source: `relational_functions.h`*

---

### INNER_JOIN

**Signature:** `INNER_JOIN(left, right, leftKey, rightKey)` → `array`  

Perform inner join on two arrays of objects

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `left` | `array` | ✅ | Left array |
| `right` | `array` | ✅ | Right array |
| `leftKey` | `string` | ✅ | Key field in left array |
| `rightKey` | `string` | ✅ | Key field in right array |

**Examples:**

```aql
INNER_JOIN(orders, customers, 'customerId', 'id')
```

*Source: `relational_functions.h`*

---

### LAG

**Signature:** `LAG(array, field?, offset?, default_value?)` → `array`  

Add lagged (previous) values to array elements

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of values or objects |
| `field` | `string` | — | Field name (for objects) |
| `offset` | `integer` | — | Number of rows to look back |
| `default_value` | `any` | — | Default for null values |

**Examples:**

```aql
LAG([1,2,3,4,5])  // Returns [null,1,2,3,4]
```

*Source: `relational_functions.h`*

---

### LEAD

**Signature:** `LEAD(array, field?, offset?, default_value?)` → `array`  

Add leading (next) values to array elements

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of values or objects |
| `field` | `string` | — | Field name (for objects) |
| `offset` | `integer` | — | Number of rows to look ahead |
| `default_value` | `any` | — | Default for null values |

**Examples:**

```aql
LEAD([1,2,3,4,5])  // Returns [2,3,4,5,null]
```

*Source: `relational_functions.h`*

---

### LEAST

**Signature:** `LEAST(values)` → `any`  

Return the least (minimum) value from arguments

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | ✅ | Values to compare (variadic) |

**Examples:**

```aql
LEAST(1, 5, 3)  // Returns 1
```

*Source: `relational_functions.h`*

---

### LEFT_JOIN

**Signature:** `LEFT_JOIN(left, right, leftKey, rightKey)` → `array`  

Perform left outer join on two arrays of objects

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `left` | `array` | ✅ | Left array |
| `right` | `array` | ✅ | Right array |
| `leftKey` | `string` | ✅ | Key field in left array |
| `rightKey` | `string` | ✅ | Key field in right array |

**Examples:**

```aql
LEFT_JOIN(orders, customers, 'customerId', 'id')
```

*Source: `relational_functions.h`*

---

### LOOKUP

**Signature:** `LOOKUP(array, key, value)` → `any`  

Find first object in array where key equals value

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of objects |
| `key` | `string` | ✅ | Field name to match |
| `value` | `any` | ✅ | Value to find |

**Examples:**

```aql
LOOKUP(users, 'id', 123)
```

*Source: `relational_functions.h`*

---

### MEDIAN

**Signature:** `MEDIAN(array)` → `number`    
**Aggregate:** ✅

Calculate median (50th percentile)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of numbers |

**Examples:**

```aql
MEDIAN([1, 2, 3, 4, 5])  // Returns 3
```

*Source: `relational_functions.h`*

---

### NTILE

**Signature:** `NTILE(array, n)` → `array`  

Divide array into n equal-sized buckets

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array to partition |
| `n` | `integer` | ✅ | Number of buckets |

**Examples:**

```aql
NTILE([1,2,3,4,5,6], 3)  // Assigns bucket 1,1,2,2,3,3
```

*Source: `relational_functions.h`*

---

### NULLIF

**Signature:** `NULLIF(value1, value2)` → `any`  

Return null if both values are equal, otherwise return first value

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value1` | `any` | ✅ | First value |
| `value2` | `any` | ✅ | Second value |

**Examples:**

```aql
NULLIF(5, 5)  // Returns null
NULLIF(5, 3)  // Returns 5
```

*Source: `relational_functions.h`*

---

### PERCENTILE

**Signature:** `PERCENTILE(array, percentile)` → `number`    
**Aggregate:** ✅

Calculate percentile value (0-100)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of numbers |
| `percentile` | `number` | ✅ | Percentile (0-100) |

**Examples:**

```aql
PERCENTILE([1,2,3,4,5,6,7,8,9,10], 90)  // Returns 9
```

*Source: `relational_functions.h`*

---

### ROW_NUMBER

**Signature:** `ROW_NUMBER(array)` → `array`  

Add sequential row numbers to array elements

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of objects |

**Examples:**

```aql
ROW_NUMBER([{name:'a'},{name:'b'}])
```

*Source: `relational_functions.h`*

---

### RUNNING_SUM

**Signature:** `RUNNING_SUM(array, field?)` → `array`  

Calculate running (cumulative) sum

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of numbers or objects |
| `field` | `string` | — | Field name for objects |

**Examples:**

```aql
RUNNING_SUM([1,2,3,4,5])  // Returns [1,3,6,10,15]
```

*Source: `relational_functions.h`*

---

### STDDEV

**Signature:** `STDDEV(array)` → `number`    
**Aggregate:** ✅

Calculate population standard deviation

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of numbers |

**Examples:**

```aql
STDDEV([1, 2, 3, 4, 5])
```

*Source: `relational_functions.h`*

---

### STDDEV_SAMPLE

**Signature:** `STDDEV_SAMPLE(array)` → `number`    
**Aggregate:** ✅

Calculate sample standard deviation (n-1 denominator)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of numbers |

**Examples:**

```aql
STDDEV_SAMPLE([1, 2, 3, 4, 5])
```

*Source: `relational_functions.h`*

---

### VARIANCE

**Signature:** `VARIANCE(array)` → `number`    
**Aggregate:** ✅

Calculate population variance

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array of numbers |

**Examples:**

```aql
VARIANCE([1, 2, 3, 4, 5])
```

*Source: `relational_functions.h`*

---

## File Functions

| Function | Description |
|----------|-------------|
| [FILENAME](#filename) | Get filename from path (alias for PATH_BASENAME) |
| [FILENAME_WITHOUT_EXT](#filename_without_ext) | Get filename without extension |
| [FILE_EXT](#file_ext) | Get file extension including the dot |
| [FORMAT_FILESIZE](#format_filesize) | Format byte count as human-readable size (KB, MB, GB, etc.) |
| [IS_AUDIO](#is_audio) | Check if file has an audio extension |
| [IS_DOCUMENT](#is_document) | Check if file has a document extension |
| [IS_IMAGE](#is_image) | Check if file has an image extension |
| [IS_VIDEO](#is_video) | Check if file has a video extension |
| [MIME_TYPE](#mime_type) | Get MIME type based on file extension |
| [PARSE_FILESIZE](#parse_filesize) | Parse human-readable size string to bytes |
| [PATH_BASENAME](#path_basename) | Extract the filename portion of a path |
| [PATH_DIRNAME](#path_dirname) | Extract the directory portion of a path |
| [PATH_EXTENSION](#path_extension) | Extract the file extension (without dot) |
| [PATH_IS_ABSOLUTE](#path_is_absolute) | Check if path is absolute (starts with / or drive letter) |
| [PATH_IS_RELATIVE](#path_is_relative) | Check if path is relative |
| [PATH_JOIN](#path_join) | Join multiple path components with proper separators |
| [PATH_NORMALIZE](#path_normalize) | Normalize path by resolving . and .. and standardizing separators |
| [PATH_PARENT](#path_parent) | Get parent directory, optionally multiple levels up |
| [PATH_SPLIT](#path_split) | Split path into array of components |
| [SANITIZE_FILENAME](#sanitize_filename) | Remove or replace unsafe characters from filename |

### FILENAME

**Signature:** `FILENAME(path)` → `string`  

Get filename from path (alias for PATH_BASENAME)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |

**Examples:**

```aql
FILENAME('/path/to/document.pdf')  // 'document.pdf'
```

*Source: `file_functions.h`*

---

### FILENAME_WITHOUT_EXT

**Signature:** `FILENAME_WITHOUT_EXT(path)` → `string`  

Get filename without extension

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |

**Examples:**

```aql
FILENAME_WITHOUT_EXT('/path/to/document.pdf')  // 'document'
```

*Source: `file_functions.h`*

---

### FILE_EXT

**Signature:** `FILE_EXT(path)` → `string`  

Get file extension including the dot

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |

**Examples:**

```aql
FILE_EXT('/path/to/document.pdf')  // '.pdf'
```

*Source: `file_functions.h`*

---

### FORMAT_FILESIZE

**Signature:** `FORMAT_FILESIZE(bytes, precision?)` → `string`  

Format byte count as human-readable size (KB, MB, GB, etc.)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `bytes` | `number` | ✅ | Size in bytes |
| `precision` | `integer` | — | Decimal places |

**Examples:**

```aql
FORMAT_FILESIZE(1536000)  // '1.46 MB'
```

*Source: `file_functions.h`*

---

### IS_AUDIO

**Signature:** `IS_AUDIO(path)` → `boolean`  

Check if file has an audio extension

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |

**Examples:**

```aql
IS_AUDIO('song.mp3')  // true
```

*Source: `file_functions.h`*

---

### IS_DOCUMENT

**Signature:** `IS_DOCUMENT(path)` → `boolean`  

Check if file has a document extension

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |

**Examples:**

```aql
IS_DOCUMENT('report.pdf')  // true
```

*Source: `file_functions.h`*

---

### IS_IMAGE

**Signature:** `IS_IMAGE(path)` → `boolean`  

Check if file has an image extension

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |

**Examples:**

```aql
IS_IMAGE('photo.jpg')  // true
```

*Source: `file_functions.h`*

---

### IS_VIDEO

**Signature:** `IS_VIDEO(path)` → `boolean`  

Check if file has a video extension

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |

**Examples:**

```aql
IS_VIDEO('movie.mp4')  // true
```

*Source: `file_functions.h`*

---

### MIME_TYPE

**Signature:** `MIME_TYPE(path)` → `string`  

Get MIME type based on file extension

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path or extension |

**Examples:**

```aql
MIME_TYPE('document.pdf')  // 'application/pdf'
```

*Source: `file_functions.h`*

---

### PARSE_FILESIZE

**Signature:** `PARSE_FILESIZE(size_string)` → `number`  

Parse human-readable size string to bytes

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `size_string` | `string` | ✅ | Size like '1.5 MB' or '100 KB' |

**Examples:**

```aql
PARSE_FILESIZE('1.5 MB')  // 1572864
```

*Source: `file_functions.h`*

---

### PATH_BASENAME

**Signature:** `PATH_BASENAME(path, strip_extension?)` → `string`  

Extract the filename portion of a path

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |
| `strip_extension` | `boolean` | — | Remove extension |

**Examples:**

```aql
PATH_BASENAME('/home/user/file.txt')  // 'file.txt'
```

*Source: `file_functions.h`*

---

### PATH_DIRNAME

**Signature:** `PATH_DIRNAME(path)` → `string`  

Extract the directory portion of a path

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |

**Examples:**

```aql
PATH_DIRNAME('/home/user/file.txt')  // '/home/user'
```

*Source: `file_functions.h`*

---

### PATH_EXTENSION

**Signature:** `PATH_EXTENSION(path)` → `string`  

Extract the file extension (without dot)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |

**Examples:**

```aql
PATH_EXTENSION('/home/user/file.txt')  // 'txt'
```

*Source: `file_functions.h`*

---

### PATH_IS_ABSOLUTE

**Signature:** `PATH_IS_ABSOLUTE(path)` → `boolean`  

Check if path is absolute (starts with / or drive letter)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | Path to check |

**Examples:**

```aql
PATH_IS_ABSOLUTE('/home/user')  // true
```

*Source: `file_functions.h`*

---

### PATH_IS_RELATIVE

**Signature:** `PATH_IS_RELATIVE(path)` → `boolean`  

Check if path is relative

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | Path to check |

**Examples:**

```aql
PATH_IS_RELATIVE('docs/file.txt')  // true
```

*Source: `file_functions.h`*

---

### PATH_JOIN

**Signature:** `PATH_JOIN(paths)` → `string`  

Join multiple path components with proper separators

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `paths` | `any` | ✅ | Path components to join (variadic) |

**Examples:**

```aql
PATH_JOIN('/home', 'user', 'docs')  // '/home/user/docs'
```

*Source: `file_functions.h`*

---

### PATH_NORMALIZE

**Signature:** `PATH_NORMALIZE(path)` → `string`  

Normalize path by resolving . and .. and standardizing separators

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | Path to normalize |

**Examples:**

```aql
PATH_NORMALIZE('/home/user/../admin/./docs')  // '/home/admin/docs'
```

*Source: `file_functions.h`*

---

### PATH_PARENT

**Signature:** `PATH_PARENT(path, levels?)` → `string`  

Get parent directory, optionally multiple levels up

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | File path |
| `levels` | `integer` | — | Number of levels to go up |

**Examples:**

```aql
PATH_PARENT('/home/user/docs/file.txt', 2)  // '/home/user'
```

*Source: `file_functions.h`*

---

### PATH_SPLIT

**Signature:** `PATH_SPLIT(path)` → `array`  

Split path into array of components

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `path` | `string` | ✅ | Path to split |

**Examples:**

```aql
PATH_SPLIT('/home/user/docs')  // ['/', 'home', 'user', 'docs']
```

*Source: `file_functions.h`*

---

### SANITIZE_FILENAME

**Signature:** `SANITIZE_FILENAME(filename, replacement?)` → `string`  

Remove or replace unsafe characters from filename

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `filename` | `string` | ✅ | Filename to sanitize |
| `replacement` | `string` | — | Replacement character |

**Examples:**

```aql
SANITIZE_FILENAME('file:name?.txt')  // 'file_name_.txt'
```

*Source: `file_functions.h`*

---

## Collection Functions

| Function | Description |
|----------|-------------|
| [ARRAY](#array) | Creates an array from arguments. Supports JSON-native string parsing. |
| [DICT](#dict) | Creates an object from key-value pairs or JSON string |
| [ENTRIES](#entries) | Returns object entries as array of [key, value] pairs |
| [FROM_ENTRIES](#from_entries) | Creates an object from an array of [key, value] pairs |
| [HOLIDAYS](#holidays) | Load holidays from calendar(s) or create from date strings |
| [HOLIDAYS_BETWEEN](#holidays_between) | Returns holidays from a calendar within a date range |
| [JSON](#json) | Parses a JSON string into a native value |
| [JSON_TYPE](#json_type) | Returns the JSON type of a value as a string |
| [JSON_VALID](#json_valid) | Returns true if the string is valid JSON |
| [KEYS](#keys) | Returns the keys of an object as an array |
| [LIST](#list) | Converts a value to a list/array |
| [LIST_CALENDARS](#list_calendars) | Lists all available holiday calendar names |
| [LOAD_HOLIDAYS](#load_holidays) | Securely loads a holiday calendar from external YAML/JSON file |
| [PAIR](#pair) | Creates a key-value pair as a two-element array |
| [RANGE](#range) | Creates an array of numbers from start to end (exclusive) |
| [REPEAT](#repeat) | Creates an array with a value repeated N times |
| [SET](#set) | Creates an array with unique values only (like a mathematical set) |
| [TO_JSON](#to_json) | Serializes a value to a JSON string |
| [TUPLE](#tuple) | Creates a tuple (fixed-size array) from arguments |

### ARRAY

**Signature:** `ARRAY(values?)` → `array`  

Creates an array from arguments. Supports JSON-native string parsing.

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | — | Values to include (variadic). JSON strings like '[1,2,3]' are parsed. |

**Examples:**

```aql
ARRAY(1, 2, 3) // [1, 2, 3]
ARRAY('[1, 2, 3]') // [1, 2, 3] - JSON parsed
ARRAY("a", "b") // ["a", "b"]
ARRAY() // []
ARRAY('[1,2]', '[3,4]') // [[1,2], [3,4]]
```

*Source: `collection_functions.h`*

---

### DICT

**Signature:** `DICT(pairs?)` → `object`  

Creates an object from key-value pairs or JSON string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `pairs` | `any` | — | Key-value pairs OR a JSON string like '{\"key\": value}' |

**Examples:**

```aql
DICT("name", "Alice", "age", 30) // {"name": "Alice", "age": 30}
DICT('{"name": "Alice"}') // {"name": "Alice"} - JSON parsed
DICT("x", 1, "y", 2) // {"x": 1, "y": 2}
DICT() // {}
```

*Source: `collection_functions.h`*

---

### ENTRIES

**Signature:** `ENTRIES(object)` → `array`  

Returns object entries as array of [key, value] pairs

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `object` | `object` | ✅ | Object to convert |

**Examples:**

```aql
ENTRIES({a: 1, b: 2}) // [["a", 1], ["b", 2]]
```

*Source: `collection_functions.h`*

---

### FROM_ENTRIES

**Signature:** `FROM_ENTRIES(entries)` → `object`  

Creates an object from an array of [key, value] pairs

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `entries` | `array` | ✅ | Array of [key, value] pairs |

**Examples:**

```aql
FROM_ENTRIES([["a", 1], ["b", 2]]) // {a: 1, b: 2}
```

*Source: `collection_functions.h`*

---

### HOLIDAYS

**Signature:** `HOLIDAYS(sources?)` → `array`  

Load holidays from calendar(s) or create from date strings

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sources` | `any` | — | Calendar names or date strings (YYYY-MM-DD) |

**Examples:**

```aql
HOLIDAYS("DE_2024") // Load German holidays
HOLIDAYS("DE_2024", "AT_2024") // Merge calendars
HOLIDAYS("2024-12-25", "2024-12-26") // Inline dates
WORKDAYS(start, end, HOLIDAYS("DE_2024"))
```

*Source: `collection_functions.h`*

---

### HOLIDAYS_BETWEEN

**Signature:** `HOLIDAYS_BETWEEN(calendarName, startDate, endDate)` → `array`  

Returns holidays from a calendar within a date range

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `calendarName` | `string` | ✅ | Calendar name (e.g., 'DE_2024') |
| `startDate` | `integer` | ✅ | Start timestamp (ms) |
| `endDate` | `integer` | ✅ | End timestamp (ms) |

**Examples:**

```aql
HOLIDAYS_BETWEEN("DE_2024", MAKE_DATE(2024,12,1), MAKE_DATE(2024,12,31))
```

*Source: `collection_functions.h`*

---

### JSON

**Signature:** `JSON(jsonString)` → `any`  

Parses a JSON string into a native value

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `jsonString` | `string` | ✅ | JSON string to parse |

**Examples:**

```aql
JSON('[1, 2, 3]') // [1, 2, 3]
JSON('{"name": "Alice"}') // {"name": "Alice"}
JSON('null') // null
JSON('123') // 123
```

*Source: `collection_functions.h`*

---

### JSON_TYPE

**Signature:** `JSON_TYPE(value)` → `string`  

Returns the JSON type of a value as a string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to check |

**Examples:**

```aql
JSON_TYPE([1, 2]) // "array"
JSON_TYPE({a: 1}) // "object"
JSON_TYPE(123) // "number"
JSON_TYPE("text") // "string"
JSON_TYPE(null) // "null"
JSON_TYPE(true) // "boolean"
```

*Source: `collection_functions.h`*

---

### JSON_VALID

**Signature:** `JSON_VALID(jsonString)` → `boolean`  

Returns true if the string is valid JSON

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `jsonString` | `string` | ✅ | String to validate |

**Examples:**

```aql
JSON_VALID('[1, 2, 3]') // true
JSON_VALID('not json') // false
```

*Source: `collection_functions.h`*

---

### KEYS

**Signature:** `KEYS(object)` → `array`  

Returns the keys of an object as an array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `object` | `object` | ✅ | Object to get keys from |

**Examples:**

```aql
KEYS({a: 1, b: 2, c: 3}) // ["a", "b", "c"]
```

*Source: `collection_functions.h`*

---

### LIST

**Signature:** `LIST(value)` → `array`  

Converts a value to a list/array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to convert |

**Examples:**

```aql
LIST("a,b,c") // ["a", "b", "c"]
LIST({a: 1, b: 2}) // [1, 2]
LIST([1, 2, 3]) // [1, 2, 3]
```

*Source: `collection_functions.h`*

---

### LIST_CALENDARS

**Signature:** `LIST_CALENDARS()` → `array`    
**Non-deterministic** (result may vary)

Lists all available holiday calendar names

**Examples:**

```aql
LIST_CALENDARS() // ["germany_2024", "us_federal_2024", ...]
```

*Source: `collection_functions.h`*

---

### LOAD_HOLIDAYS

**Signature:** `LOAD_HOLIDAYS(calendarName)` → `array`    
**Non-deterministic** (result may vary)

Securely loads a holiday calendar from external YAML/JSON file

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `calendarName` | `string` | ✅ | Calendar name (e.g., 'germany_2024', 'us_federal_2024') |

**Examples:**

```aql
LET holidays = LOAD_HOLIDAYS("germany_2024")
WORKDAYS(start, end, LOAD_HOLIDAYS("company_holidays"))
```

*Source: `collection_functions.h`*

---

### PAIR

**Signature:** `PAIR(key, value)` → `array`  

Creates a key-value pair as a two-element array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `key` | `any` | ✅ | The key |
| `value` | `any` | ✅ | The value |

**Examples:**

```aql
PAIR("name", "Alice") // ["name", "Alice"]
PAIR(1, "one") // [1, "one"]
```

*Source: `collection_functions.h`*

---

### RANGE

**Signature:** `RANGE(start, end, step?)` → `array`  

Creates an array of numbers from start to end (exclusive)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `start` | `number` | ✅ | Start value (inclusive) |
| `end` | `number` | ✅ | End value (exclusive) |
| `step` | `number` | — | Step size (default: 1) |

**Examples:**

```aql
RANGE(0, 5) // [0, 1, 2, 3, 4]
RANGE(1, 10, 2) // [1, 3, 5, 7, 9]
RANGE(10, 0, -1) // [10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
```

*Source: `collection_functions.h`*

---

### REPEAT

**Signature:** `REPEAT(value, count)` → `array`  

Creates an array with a value repeated N times

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to repeat |
| `count` | `integer` | ✅ | Number of repetitions |

**Examples:**

```aql
REPEAT(0, 5) // [0, 0, 0, 0, 0]
REPEAT("x", 3) // ["x", "x", "x"]
```

*Source: `collection_functions.h`*

---

### SET

**Signature:** `SET(values?)` → `array`  

Creates an array with unique values only (like a mathematical set)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | — | Values to include (duplicates removed) |

**Examples:**

```aql
SET(1, 2, 2, 3) // [1, 2, 3]
SET("a", "b", "a") // ["a", "b"]
SET([1, 1, 2, 2]) // [1, 2]
```

*Source: `collection_functions.h`*

---

### TO_JSON

**Signature:** `TO_JSON(value, pretty?)` → `string`  

Serializes a value to a JSON string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to serialize |
| `pretty` | `boolean` | — | Pretty-print with indentation (default: false) |

**Examples:**

```aql
TO_JSON([1, 2, 3]) // "[1,2,3]"
TO_JSON({name: "Alice"}) // '{"name":"Alice"}'
TO_JSON({a: 1}, true) // Pretty-printed
```

*Source: `collection_functions.h`*

---

### TUPLE

**Signature:** `TUPLE(values?)` → `array`  

Creates a tuple (fixed-size array) from arguments

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | — | Values for the tuple |

**Examples:**

```aql
TUPLE(1, "a", true) // [1, "a", true]
TUPLE(x, y) // [x, y]
```

*Source: `collection_functions.h`*

---

## Ethics Functions

| Function | Description |
|----------|-------------|
| [ETHICS_BUILD_CONTEXT](#ethics_build_context) | Build RAG context by retrieving similar dilemmas, arguments, and best practices |
| [ETHICS_EVALUATE](#ethics_evaluate) | Evaluate ethical decision quality across 5 dimensions: quality, consistency, fairness, alignment, transparency |
| [ETHICS_EVALUATE_DIMENSION](#ethics_evaluate_dimension) | Evaluate a specific dimension of an ethical decision |
| [ETHICS_FIND_SIMILAR_DILEMMAS](#ethics_find_similar_dilemmas) | Find similar ethical dilemmas using vector similarity search |
| [ETHICS_GET_ARGUMENTS](#ethics_get_arguments) | Retrieve ethical arguments filtered by philosophy school and type |
| [ETHICS_INITIALIZE_DEBATE](#ethics_initialize_debate) | Initialize a multi-philosophy ethical debate session |
| [ETHICS_LIST_SCHOOLS](#ethics_list_schools) | List all available philosophy schools |
| [ETHICS_LOAD_PROFILE](#ethics_load_profile) | Load detailed philosophy profile by school name |
| [ETHICS_MAKE_DECISION](#ethics_make_decision) | Make ethical decision using multi-philosophy analysis with optional RAG context |
| [ETHICS_METRICS](#ethics_metrics) | Get Ethics AI system metrics in Prometheus format |
| [ETHICS_STATS](#ethics_stats) | Get statistics for a philosophy school (argument count, decision count, etc.) |
| [ETHICS_TRAVERSE_CHAIN](#ethics_traverse_chain) | Traverse argument chains using graph relationships (supports/counters/rebuts) |

### ETHICS_BUILD_CONTEXT

**Signature:** `ETHICS_BUILD_CONTEXT(dilemma_description, philosophy_schools, category?)` → `object`  

Build RAG context by retrieving similar dilemmas, arguments, and best practices

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `dilemma_description` | `string` | ✅ | Current ethical dilemma |
| `philosophy_schools` | `array` | ✅ | Philosophy schools to include |
| `category` | `string` | — | Dilemma category |

**Examples:**

```aql
ETHICS_BUILD_CONTEXT('AI decision-making ethics', ['kant'], 'ai_systems')
```

*Source: `ethics_functions.h`*

---

### ETHICS_EVALUATE

**Signature:** `ETHICS_EVALUATE(decision, arguments?)` → `object`  

Evaluate ethical decision quality across 5 dimensions: quality, consistency, fairness, alignment, transparency

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `decision` | `object` | ✅ | Decision object to evaluate |
| `arguments` | `array` | — | Optional list of arguments used in decision |

**Examples:**

```aql
ETHICS_EVALUATE(decision, [])
ETHICS_EVALUATE(decision, arguments)
```

*Source: `ethics_functions.h`*

---

### ETHICS_EVALUATE_DIMENSION

**Signature:** `ETHICS_EVALUATE_DIMENSION(decision, dimension)` → `number`  

Evaluate a specific dimension of an ethical decision

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `decision` | `object` | ✅ | Decision object |
| `dimension` | `string` | ✅ | Dimension: decision_quality, consistency, fairness, alignment, transparency |

**Examples:**

```aql
ETHICS_EVALUATE_DIMENSION(decision, 'fairness')
ETHICS_EVALUATE_DIMENSION(decision, 'alignment')
```

*Source: `ethics_functions.h`*

---

### ETHICS_FIND_SIMILAR_DILEMMAS

**Signature:** `ETHICS_FIND_SIMILAR_DILEMMAS(query_text, threshold?, limit?)` → `array`  

Find similar ethical dilemmas using vector similarity search

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `query_text` | `string` | ✅ | Query text to find similar dilemmas |
| `threshold` | `number` | — | Similarity threshold (0-1) |
| `limit` | `integer` | — | Maximum number of results |

**Examples:**

```aql
ETHICS_FIND_SIMILAR_DILEMMAS('AI privacy vs security', 0.7, 5)
```

*Source: `ethics_functions.h`*

---

### ETHICS_GET_ARGUMENTS

**Signature:** `ETHICS_GET_ARGUMENTS(philosophy_school, argument_types?, limit?)` → `array`  

Retrieve ethical arguments filtered by philosophy school and type

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `philosophy_school` | `string` | ✅ | Philosophy school: kant, utilitarianism, virtue_ethics, etc. |
| `argument_types` | `array` | — | Filter by types: pro, contra, rebuttal, synthesis (empty = all) |
| `limit` | `integer` | — | Maximum number of arguments to return |

**Examples:**

```aql
ETHICS_GET_ARGUMENTS('kant', [], 10)
ETHICS_GET_ARGUMENTS('utilitarianism', ['pro'], 50)
```

*Source: `ethics_functions.h`*

---

### ETHICS_INITIALIZE_DEBATE

**Signature:** `ETHICS_INITIALIZE_DEBATE(dilemma_description, philosophy_schools, category?)` → `object`  

Initialize a multi-philosophy ethical debate session

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `dilemma_description` | `string` | ✅ | Description of the ethical dilemma |
| `philosophy_schools` | `array` | ✅ | List of philosophy schools to participate |
| `category` | `string` | — | Dilemma category |

**Examples:**

```aql
ETHICS_INITIALIZE_DEBATE('Should we allow gene editing?', ['kant', 'utilitarianism'], 'bioethics')
```

*Source: `ethics_functions.h`*

---

### ETHICS_LIST_SCHOOLS

**Signature:** `ETHICS_LIST_SCHOOLS()` → `array`  

List all available philosophy schools

**Examples:**

```aql
ETHICS_LIST_SCHOOLS()
```

*Source: `ethics_functions.h`*

---

### ETHICS_LOAD_PROFILE

**Signature:** `ETHICS_LOAD_PROFILE(school)` → `object`  

Load detailed philosophy profile by school name

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `school` | `string` | ✅ | Philosophy school: kant, utilitarianism, virtue_ethics, etc. |

**Examples:**

```aql
ETHICS_LOAD_PROFILE('kant')
ETHICS_LOAD_PROFILE('utilitarianism')
```

*Source: `ethics_functions.h`*

---

### ETHICS_MAKE_DECISION

**Signature:** `ETHICS_MAKE_DECISION(dilemma_description, philosophy_schools, category?, use_rag?)` → `object`  

Make ethical decision using multi-philosophy analysis with optional RAG context

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `dilemma_description` | `string` | ✅ | Description of the ethical dilemma |
| `philosophy_schools` | `array` | ✅ | List of philosophy schools to consult (e.g., ['kant', 'utilitarianism']) |
| `category` | `string` | — | Dilemma category: general, bioethics, autonomous_systems, data_ethics, etc. |
| `use_rag` | `boolean` | — | Whether to use RAG context for enhanced decision-making |

**Examples:**

```aql
ETHICS_MAKE_DECISION('Should AI prioritize privacy?', ['kant', 'utilitarianism'], 'data_ethics', true)
ETHICS_MAKE_DECISION(doc.dilemma, ['virtue_ethics'], 'general', false)
```

*Source: `ethics_functions.h`*

---

### ETHICS_METRICS

**Signature:** `ETHICS_METRICS()` → `string`  

Get Ethics AI system metrics in Prometheus format

**Examples:**

```aql
ETHICS_METRICS()
```

*Source: `ethics_functions.h`*

---

### ETHICS_STATS

**Signature:** `ETHICS_STATS(philosophy_school?)` → `object`  

Get statistics for a philosophy school (argument count, decision count, etc.)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `philosophy_school` | `string` | — | Philosophy school (empty = all schools) |

**Examples:**

```aql
ETHICS_STATS('kant')
ETHICS_STATS()
```

*Source: `ethics_functions.h`*

---

### ETHICS_TRAVERSE_CHAIN

**Signature:** `ETHICS_TRAVERSE_CHAIN(start_id, max_depth?)` → `array`  

Traverse argument chains using graph relationships (supports/counters/rebuts)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `start_id` | `string` | ✅ | Starting argument ID |
| `max_depth` | `integer` | — | Maximum traversal depth |

**Examples:**

```aql
ETHICS_TRAVERSE_CHAIN('arg_001', 3)
ETHICS_TRAVERSE_CHAIN(arg.id, 10)
```

*Source: `ethics_functions.h`*

---

## Logical Functions

| Function | Description |
|----------|-------------|
| [ALL](#all) | Returns true if all array elements are truthy |
| [AND](#and) | Returns true if all arguments are truthy (Excel-compatible) |
| [ANY](#any) | Returns true if any array element is truthy |
| [ARRAY_AND](#array_and) | Element-wise AND of two arrays |
| [ARRAY_OR](#array_or) | Element-wise OR of two arrays |
| [ARRAY_XOR](#array_xor) | Element-wise XOR of two arrays |
| [CHOOSE](#choose) | Returns value at index (1-based, Excel-compatible) |
| [COUNT_IF](#count_if) | Counts truthy elements in array (Excel COUNTIF style) |
| [FILTER_BY](#filter_by) | Filters array elements by corresponding condition array |
| [IF](#if) | Returns one value if condition is true, another if false |
| [IFERROR](#iferror) | Returns alternative value if first value is null/error |
| [IFNA](#ifna) | Returns alternative value if first value is null/N/A |
| [IFS](#ifs) | Returns value for first true condition (Excel-compatible) |
| [NONE](#none) | Returns true if no array elements are truthy |
| [NOT](#not) | Inverts truthiness of a value or array elements |
| [OR](#or) | Returns true if at least one argument is truthy (Excel-compatible) |
| [SUM_IF](#sum_if) | Sums elements where corresponding condition is truthy |
| [SWITCH](#switch) | Matches expression against cases (Excel-compatible) |
| [XOR](#xor) | Returns true if an odd number of arguments are truthy (Excel-compatible) |

### ALL

**Signature:** `ALL(array)` → `boolean`  

Returns true if all array elements are truthy

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array to test |

**Examples:**

```aql
ALL([true, true, true]) // true
ALL([1, 2, 3]) // true
ALL([1, 0, 3]) // false
```

*Source: `collection_functions.h`*

---

### AND

**Signature:** `AND(values?)` → `boolean`  

Returns true if all arguments are truthy (Excel-compatible)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | — | Values or array to test |

**Examples:**

```aql
AND(true, true, true) // true
AND(true, false) // false
AND([1, 2, 3]) // true - all non-zero
AND(1, 0, 3) // false - zero is falsy
```

*Source: `collection_functions.h`*

---

### ANY

**Signature:** `ANY(array)` → `boolean`  

Returns true if any array element is truthy

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array to test |

**Examples:**

```aql
ANY([false, false, true]) // true
ANY([0, 0, 1]) // true
ANY([0, 0, 0]) // false
```

*Source: `collection_functions.h`*

---

### ARRAY_AND

**Signature:** `ARRAY_AND(arr1, arr2)` → `array`  

Element-wise AND of two arrays

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `arr1` | `array` | ✅ | First array |
| `arr2` | `array` | ✅ | Second array |

**Examples:**

```aql
ARRAY_AND([true, true, false], [true, false, false]) // [true, false, false]
ARRAY_AND([1, 2, 0], [1, 0, 1]) // [true, false, false]
```

*Source: `collection_functions.h`*

---

### ARRAY_OR

**Signature:** `ARRAY_OR(arr1, arr2)` → `array`  

Element-wise OR of two arrays

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `arr1` | `array` | ✅ | First array |
| `arr2` | `array` | ✅ | Second array |

**Examples:**

```aql
ARRAY_OR([true, false, false], [false, false, true]) // [true, false, true]
```

*Source: `collection_functions.h`*

---

### ARRAY_XOR

**Signature:** `ARRAY_XOR(arr1, arr2)` → `array`  

Element-wise XOR of two arrays

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `arr1` | `array` | ✅ | First array |
| `arr2` | `array` | ✅ | Second array |

**Examples:**

```aql
ARRAY_XOR([true, true, false], [true, false, false]) // [false, true, false]
```

*Source: `collection_functions.h`*

---

### CHOOSE

**Signature:** `CHOOSE(index, values?)` → `any`  

Returns value at index (1-based, Excel-compatible)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `index` | `integer` | ✅ | Index (1-based) |
| `values` | `any` | — | Values to choose from |

**Examples:**

```aql
CHOOSE(2, "a", "b", "c") // "b"
CHOOSE(1, 10, 20, 30) // 10
```

*Source: `collection_functions.h`*

---

### COUNT_IF

**Signature:** `COUNT_IF(array, value?)` → `integer`  

Counts truthy elements in array (Excel COUNTIF style)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array to count |
| `value` | `any` | — | Specific value to count (optional) |

**Examples:**

```aql
COUNT_IF([true, false, true]) // 2
COUNT_IF([1, 0, 2, 0, 3]) // 3
COUNT_IF([1, 2, 1, 3, 1], 1) // 3
```

*Source: `collection_functions.h`*

---

### FILTER_BY

**Signature:** `FILTER_BY(array, conditions)` → `array`  

Filters array elements by corresponding condition array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array to filter |
| `conditions` | `array` | ✅ | Condition array (parallel) |

**Examples:**

```aql
FILTER_BY([1, 2, 3, 4], [true, false, true, false]) // [1, 3]
FILTER_BY(["a", "b", "c"], [1, 0, 1]) // ["a", "c"]
```

*Source: `collection_functions.h`*

---

### IF

**Signature:** `IF(condition, trueValue, falseValue?)` → `any`  

Returns one value if condition is true, another if false

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `condition` | `any` | ✅ | Condition to test |
| `trueValue` | `any` | ✅ | Value if true |
| `falseValue` | `any` | — | Value if false (default: null) |

**Examples:**

```aql
IF(true, "yes", "no") // "yes"
IF(x > 0, "positive", "negative")
IF(false, "yes") // null
```

*Source: `collection_functions.h`*

---

### IFERROR

**Signature:** `IFERROR(value, errorValue)` → `any`  

Returns alternative value if first value is null/error

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to test |
| `errorValue` | `any` | ✅ | Value to return if null |

**Examples:**

```aql
IFERROR(null, 0) // 0
IFERROR(123, 0) // 123
```

*Source: `collection_functions.h`*

---

### IFNA

**Signature:** `IFNA(value, naValue)` → `any`  

Returns alternative value if first value is null/N/A

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to test |
| `naValue` | `any` | ✅ | Value to return if N/A |

**Examples:**

```aql
IFNA(null, "N/A") // "N/A"
IFNA("value", "N/A") // "value"
```

*Source: `collection_functions.h`*

---

### IFS

**Signature:** `IFS(pairs?)` → `any`  

Returns value for first true condition (Excel-compatible)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `pairs` | `any` | — | Condition-value pairs |

**Examples:**

```aql
IFS(false, "a", true, "b") // "b"
IFS(x < 0, "neg", x > 0, "pos", true, "zero")
```

*Source: `collection_functions.h`*

---

### NONE

**Signature:** `NONE(array)` → `boolean`  

Returns true if no array elements are truthy

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `array` | `array` | ✅ | Array to test |

**Examples:**

```aql
NONE([false, false, false]) // true
NONE([0, 0, 0]) // true
NONE([0, 1, 0]) // false
```

*Source: `collection_functions.h`*

---

### NOT

**Signature:** `NOT(value)` → `any`  

Inverts truthiness of a value or array elements

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value or array to invert |

**Examples:**

```aql
NOT(true) // false
NOT(0) // true
NOT([true, false]) // [false, true]
```

*Source: `collection_functions.h`*

---

### OR

**Signature:** `OR(values?)` → `boolean`  

Returns true if at least one argument is truthy (Excel-compatible)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | — | Values or array to test |

**Examples:**

```aql
OR(false, true, false) // true
OR(false, false, false) // false
OR([0, 0, 1]) // true
```

*Source: `collection_functions.h`*

---

### SUM_IF

**Signature:** `SUM_IF(values, conditions)` → `number`  

Sums elements where corresponding condition is truthy

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `array` | ✅ | Array of numbers |
| `conditions` | `array` | ✅ | Array of conditions |

**Examples:**

```aql
SUM_IF([10, 20, 30], [true, false, true]) // 40
SUM_IF([1, 2, 3, 4], [1, 0, 1, 0]) // 4
```

*Source: `collection_functions.h`*

---

### SWITCH

**Signature:** `SWITCH(expression, cases?)` → `any`  

Matches expression against cases (Excel-compatible)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `expression` | `any` | ✅ | Value to match |
| `cases` | `any` | — | Case-value pairs, optional default |

**Examples:**

```aql
SWITCH(2, 1, "one", 2, "two") // "two"
SWITCH(x, "a", 1, "b", 2, 0) // 0 if no match
```

*Source: `collection_functions.h`*

---

### XOR

**Signature:** `XOR(values?)` → `boolean`  

Returns true if an odd number of arguments are truthy (Excel-compatible)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `values` | `any` | — | Values or array to test |

**Examples:**

```aql
XOR(true, false) // true
XOR(true, true) // false
XOR(true, true, true) // true
XOR([1, 0, 1]) // false
```

*Source: `collection_functions.h`*

---

## Process Functions

| Function | Description |
|----------|-------------|
| [MILESTONE_NEXT](#milestone_next) | Get next pending milestone for a case |
| [MILESTONE_OVERDUE](#milestone_overdue) | Get all overdue milestones for a case or globally |
| [MILESTONE_STATUS](#milestone_status) | Get current milestone status for a case |
| [PROCESS_CONFORMANCE](#process_conformance) | Calculate conformance score between actual and expected process |
| [PROCESS_DEVIATIONS](#process_deviations) | Find all deviations from expected process model |
| [PROCESS_PREDICT_END](#process_predict_end) | Predict when process will complete based on current progress and historical data |
| [PROCESS_PREDICT_NEXT](#process_predict_next) | Predict most likely next activity based on process model and history |
| [SLA_CHECK](#sla_check) | Check SLA compliance status for a case |
| [SLA_REMAINING](#sla_remaining) | Get remaining time until next SLA deadline |
| [WORKFLOW_ADVANCE](#workflow_advance) | Advance workflow token to next activity |
| [WORKFLOW_VARIABLES](#workflow_variables) | Get all variables for a workflow instance |

### MILESTONE_NEXT

**Signature:** `MILESTONE_NEXT(case_id)` → `object`  

Get next pending milestone for a case

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Case/Vorgang ID |

**Examples:**

```aql
MILESTONE_NEXT(\"V-2024-0001\")
```

*Source: `process_functions.h`*

---

### MILESTONE_OVERDUE

**Signature:** `MILESTONE_OVERDUE(case_id?)` → `array`  

Get all overdue milestones for a case or globally

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | — | Optional case ID (all if omitted) |

**Examples:**

```aql
MILESTONE_OVERDUE()
MILESTONE_OVERDUE(\"V-2024-0001\")
```

*Source: `process_functions.h`*

---

### MILESTONE_STATUS

**Signature:** `MILESTONE_STATUS(case_id)` → `object`  

Get current milestone status for a case

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Case/Vorgang ID |

**Examples:**

```aql
MILESTONE_STATUS(\"V-2024-0001\")
```

*Source: `process_functions.h`*

---

### PROCESS_CONFORMANCE

**Signature:** `PROCESS_CONFORMANCE(case_id, model_id?)` → `object`  

Calculate conformance score between actual and expected process

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Case ID to check |
| `model_id` | `string` | — | Process model ID (auto-detect if omitted) |

**Examples:**

```aql
PROCESS_CONFORMANCE(\"V-2024-0001\")
PROCESS_CONFORMANCE(\"V-2024-0001\", \"bauantrag_v2\")
```

*Source: `process_functions.h`*

---

### PROCESS_DEVIATIONS

**Signature:** `PROCESS_DEVIATIONS(case_id, model_id?)` → `array`  

Find all deviations from expected process model

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Case ID to check |
| `model_id` | `string` | — | Process model ID |

**Examples:**

```aql
PROCESS_DEVIATIONS(\"V-2024-0001\")
```

*Source: `process_functions.h`*

---

### PROCESS_PREDICT_END

**Signature:** `PROCESS_PREDICT_END(case_id)` → `object`  

Predict when process will complete based on current progress and historical data

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Case ID |

**Examples:**

```aql
PROCESS_PREDICT_END(\"V-2024-0001\")
```

*Source: `process_functions.h`*

---

### PROCESS_PREDICT_NEXT

**Signature:** `PROCESS_PREDICT_NEXT(case_id, top_n?)` → `array`  

Predict most likely next activity based on process model and history

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Case ID |
| `top_n` | `integer` | — | Number of predictions to return |

**Examples:**

```aql
PROCESS_PREDICT_NEXT(\"V-2024-0001\")
PROCESS_PREDICT_NEXT(\"V-2024-0001\", 5)
```

*Source: `process_functions.h`*

---

### SLA_CHECK

**Signature:** `SLA_CHECK(case_id)` → `object`  

Check SLA compliance status for a case

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Case ID |

**Examples:**

```aql
SLA_CHECK(\"V-2024-0001\")
```

*Source: `process_functions.h`*

---

### SLA_REMAINING

**Signature:** `SLA_REMAINING(case_id, milestone_id?)` → `object`  

Get remaining time until next SLA deadline

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Case ID |
| `milestone_id` | `string` | — | Specific milestone (next if omitted) |

**Examples:**

```aql
SLA_REMAINING(\"V-2024-0001\")
SLA_REMAINING(\"V-2024-0001\", \"M2\")
```

*Source: `process_functions.h`*

---

### WORKFLOW_ADVANCE

**Signature:** `WORKFLOW_ADVANCE(token_id, activity, variables?)` → `object`    
**Non-deterministic** (result may vary)

Advance workflow token to next activity

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `token_id` | `string` | ✅ | Workflow token ID |
| `activity` | `string` | ✅ | Completed activity name |
| `variables` | `object` | — | Updated variables |

**Examples:**

```aql
WORKFLOW_ADVANCE(\"T-001\", \"approve\", {approved: true})
```

*Source: `process_functions.h`*

---

### WORKFLOW_VARIABLES

**Signature:** `WORKFLOW_VARIABLES(token_id)` → `object`  

Get all variables for a workflow instance

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `token_id` | `string` | ✅ | Workflow token ID |

**Examples:**

```aql
WORKFLOW_VARIABLES(\"T-001\")
```

*Source: `process_functions.h`*

---

## ProcessMining Functions

| Function | Description |
|----------|-------------|
| [PM_BOTTLENECKS](#pm_bottlenecks) | Detect bottlenecks in process based on performance data |
| [PM_COMPARE_IDEAL](#pm_compare_ideal) | Compare a process instance with an ideal/expected model |
| [PM_CONFORMANCE](#pm_conformance) | Calculate conformance score between actual process and model |
| [PM_DEVIATIONS](#pm_deviations) | Find all deviations of a process from expected model |
| [PM_DISCOVER_PROCESS](#pm_discover_process) | Discover process model from event log using mining algorithms |
| [PM_EXPORT_BPMN](#pm_export_bpmn) | Export process model as BPMN 2.0 XML |
| [PM_EXTRACT_LOG](#pm_extract_log) | Extract event log from a collection for process mining |
| [PM_EXTRACT_TRACE](#pm_extract_trace) | Extract the execution trace (activity sequence) for a specific case |
| [PM_FIND_SIMILAR](#pm_find_similar) | Find processes similar to a given pattern using graph, vector, or hybrid similarity |
| [PM_HAS_PATTERN](#pm_has_pattern) | Check if a process instance matches a specific pattern |
| [PM_LIST_ADMIN_MODELS](#pm_list_admin_models) | List all available predefined administrative process models |
| [PM_LOAD_ADMIN_MODEL](#pm_load_admin_model) | Load a predefined administrative process model (Bauantrag, Beschaffung, HR, etc.) |
| [PM_PREDICT_END](#pm_predict_end) | Predict when a running process will complete |
| [PM_VARIANTS](#pm_variants) | Analyze and return top process variants from event log |

### PM_BOTTLENECKS

**Signature:** `PM_BOTTLENECKS(event_log, threshold_percentile?)` → `array`  

Detect bottlenecks in process based on performance data

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `event_log` | `object` | ✅ | Event log with timing data |
| `threshold_percentile` | `number` | — | Percentile threshold for bottleneck detection (0-1) |

**Examples:**

```aql
PM_BOTTLENECKS(log)
PM_BOTTLENECKS(log, 0.95)
```

*Source: `process_mining_functions.h`*

---

### PM_COMPARE_IDEAL

**Signature:** `PM_COMPARE_IDEAL(case_id, ideal_model)` → `object`  

Compare a process instance with an ideal/expected model

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Process case ID to compare |
| `ideal_model` | `object` | ✅ | Ideal process pattern or model ID |

**Examples:**

```aql
PM_COMPARE_IDEAL('V-2024-0001', ideal_pattern)
PM_COMPARE_IDEAL(case.id, PM_LOAD_ADMIN_MODEL('bauantrag_standard'))
```

*Source: `process_mining_functions.h`*

---

### PM_CONFORMANCE

**Signature:** `PM_CONFORMANCE(case_id, model)` → `object`  

Calculate conformance score between actual process and model

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Process case ID |
| `model` | `object` | ✅ | Process model or model ID |

**Examples:**

```aql
PM_CONFORMANCE('V-001', model)
PM_CONFORMANCE(case.id, PM_LOAD_ADMIN_MODEL('bauantrag_standard'))
```

*Source: `process_mining_functions.h`*

---

### PM_DEVIATIONS

**Signature:** `PM_DEVIATIONS(case_id, model)` → `array`  

Find all deviations of a process from expected model

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Process case ID |
| `model` | `object` | ✅ | Process model or model ID |

**Examples:**

```aql
PM_DEVIATIONS('V-001', model)
```

*Source: `process_mining_functions.h`*

---

### PM_DISCOVER_PROCESS

**Signature:** `PM_DISCOVER_PROCESS(event_log, config?)` → `object`  

Discover process model from event log using mining algorithms

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `event_log` | `object` | ✅ | Event log object |
| `config` | `object` | — | Mining configuration: algorithm (alpha|heuristic|inductive), thresholds |

**Examples:**

```aql
PM_DISCOVER_PROCESS(log, {algorithm: 'alpha'})
PM_DISCOVER_PROCESS(log, {algorithm: 'heuristic', dependency_threshold: 0.9})
```

*Source: `process_mining_functions.h`*

---

### PM_EXPORT_BPMN

**Signature:** `PM_EXPORT_BPMN(model)` → `string`  

Export process model as BPMN 2.0 XML

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `model` | `object` | ✅ | Process model to export |

**Examples:**

```aql
PM_EXPORT_BPMN(discovered_model)
```

*Source: `process_mining_functions.h`*

---

### PM_EXTRACT_LOG

**Signature:** `PM_EXTRACT_LOG(collection, config)` → `object`  

Extract event log from a collection for process mining

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `collection` | `string` | ✅ | Collection name |
| `config` | `object` | ✅ | Configuration with case_id_field, activity_field, timestamp_field |

**Examples:**

```aql
PM_EXTRACT_LOG('audit', {case_id_field: 'order_id', activity_field: 'action', timestamp_field: 'ts'})
```

*Source: `process_mining_functions.h`*

---

### PM_EXTRACT_TRACE

**Signature:** `PM_EXTRACT_TRACE(case_id)` → `object`  

Extract the execution trace (activity sequence) for a specific case

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Process case ID |

**Examples:**

```aql
PM_EXTRACT_TRACE('V-2024-0001')
```

*Source: `process_mining_functions.h`*

---

### PM_FIND_SIMILAR

**Signature:** `PM_FIND_SIMILAR(pattern, config?)` → `array`  

Find processes similar to a given pattern using graph, vector, or hybrid similarity

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `pattern` | `object` | ✅ | Process pattern with activities and edges |
| `config` | `object` | — | Configuration: method (graph|vector|behavioral|hybrid), threshold, limit |

**Examples:**

```aql
PM_FIND_SIMILAR({activities: ['A', 'B', 'C']}, {method: 'graph', threshold: 0.8})
PM_FIND_SIMILAR(ideal_pattern, {method: 'hybrid', limit: 20})
```

*Source: `process_mining_functions.h`*

---

### PM_HAS_PATTERN

**Signature:** `PM_HAS_PATTERN(case_id, pattern, threshold?)` → `boolean`  

Check if a process instance matches a specific pattern

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Process case ID |
| `pattern` | `object` | ✅ | Pattern to check |
| `threshold` | `number` | — | Minimum similarity threshold (0-1) |

**Examples:**

```aql
PM_HAS_PATTERN('V-001', {activities: ['A', 'B']})
PM_HAS_PATTERN(case.id, pattern, 0.9)
```

*Source: `process_mining_functions.h`*

---

### PM_LIST_ADMIN_MODELS

**Signature:** `PM_LIST_ADMIN_MODELS()` → `array`  

List all available predefined administrative process models

**Examples:**

```aql
PM_LIST_ADMIN_MODELS()
```

*Source: `process_mining_functions.h`*

---

### PM_LOAD_ADMIN_MODEL

**Signature:** `PM_LOAD_ADMIN_MODEL(model_id)` → `object`  

Load a predefined administrative process model (Bauantrag, Beschaffung, HR, etc.)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `model_id` | `string` | ✅ | Model ID: bauantrag_standard, beschaffung_vergaberecht, personal_einstellung, etc. |

**Examples:**

```aql
PM_LOAD_ADMIN_MODEL('bauantrag_standard')
PM_LOAD_ADMIN_MODEL('beschaffung_vergaberecht')
```

*Source: `process_mining_functions.h`*

---

### PM_PREDICT_END

**Signature:** `PM_PREDICT_END(case_id)` → `object`  

Predict when a running process will complete

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `case_id` | `string` | ✅ | Process case ID |

**Examples:**

```aql
PM_PREDICT_END('V-2024-0001')
```

*Source: `process_mining_functions.h`*

---

### PM_VARIANTS

**Signature:** `PM_VARIANTS(event_log, top_n?)` → `array`  

Analyze and return top process variants from event log

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `event_log` | `object` | ✅ | Event log object |
| `top_n` | `integer` | — | Number of top variants to return |

**Examples:**

```aql
PM_VARIANTS(log)
PM_VARIANTS(log, 10)
```

*Source: `process_mining_functions.h`*

---

## Retention Functions

| Function | Description |
|----------|-------------|
| [ESTIMATE_STORAGE_SAVINGS](#estimate_storage_savings) | Estimates storage savings from downsampling |
| [RETENTION_RESOLUTION](#retention_resolution) | Suggests data retention resolution based on variance (CV) |

### ESTIMATE_STORAGE_SAVINGS

**Signature:** `ESTIMATE_STORAGE_SAVINGS(sourceResolution, targetResolution, dataPoints)` → `object`  

Estimates storage savings from downsampling

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `sourceResolution` | `string` | ✅ | Source resolution (e.g., '1s') |
| `targetResolution` | `string` | ✅ | Target resolution (e.g., '1h') |
| `dataPoints` | `integer` | ✅ | Number of data points |

**Examples:**

```aql
ESTIMATE_STORAGE_SAVINGS('1s', '1h', 31536000) // 1 year of 1s data -> 1h
```

*Source: `retention_functions.h`*

---

### RETENTION_RESOLUTION

**Signature:** `RETENTION_RESOLUTION(cv, lowThreshold?, mediumThreshold?)` → `string`  

Suggests data retention resolution based on variance (CV)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `cv` | `number` | ✅ | Coefficient of Variation |
| `lowThreshold` | `number` | — | Low CV threshold (default: 5.0) |
| `mediumThreshold` | `number` | — | Medium CV threshold (default: 20.0) |

**Examples:**

```aql
RETENTION_RESOLUTION(3.5) // '1h' (low variance)
RETENTION_RESOLUTION(15) // '15m' (medium variance)
RETENTION_RESOLUTION(25) // '1m' (high variance)
```

*Source: `retention_functions.h`*

---

## Scheduling Functions

| Function | Description |
|----------|-------------|
| [CANCEL_TASK](#cancel_task) | ⚠️ ADMIN ONLY: Cancels a scheduled task |
| [LIST_SCHEDULED_TASKS](#list_scheduled_tasks) | Lists all scheduled tasks |
| [SCHEDULE_TASK](#schedule_task) | ⚠️ ADMIN ONLY: Creates a scheduled task that runs periodically |

### CANCEL_TASK

**Signature:** `CANCEL_TASK(taskId)` → `boolean`    
**Non-deterministic** (result may vary)

⚠️ ADMIN ONLY: Cancels a scheduled task

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `taskId` | `string` | ✅ | Task ID to cancel |

**Examples:**

```aql
CANCEL_TASK('task_12345') // Returns true if cancelled
```

*Source: `retention_functions.h`*

---

### LIST_SCHEDULED_TASKS

**Signature:** `LIST_SCHEDULED_TASKS()` → `array`    
**Non-deterministic** (result may vary)

Lists all scheduled tasks

**Examples:**

```aql
LIST_SCHEDULED_TASKS() // Returns array of task objects
```

*Source: `retention_functions.h`*

---

### SCHEDULE_TASK

**Signature:** `SCHEDULE_TASK(config)` → `object`    
**Non-deterministic** (result may vary)

⚠️ ADMIN ONLY: Creates a scheduled task that runs periodically

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `config` | `object` | ✅ | Task configuration object |

**Examples:**

```aql
SCHEDULE_TASK({
                    name: 'Daily Cleanup',
                    type: 'aql',
                    query: 'FOR d IN timeseries FILTER d.timestamp < DATE_SUB(NOW(), 30, "days") REMOVE d IN timeseries',
                    interval_hours: 24
                })
SCHEDULE_TASK({
                    name: 'Hourly Aggregation',
                    type: 'aql',
                    query: 'FOR d IN timeseries ... INSERT INTO aggregates',
                    interval_minutes: 60
                })
```

*Source: `retention_functions.h`*

---

## Statistics Functions

| Function | Description |
|----------|-------------|
| [CV](#cv) | Calculates Coefficient of Variation (CV = stddev/mean × 100%) |
| [VARIANCE_LEVEL](#variance_level) | Classifies variance level based on CV thresholds |

### CV

**Signature:** `CV(stddev, mean)` → `number`  

Calculates Coefficient of Variation (CV = stddev/mean × 100%)

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `stddev` | `number` | ✅ | Standard deviation |
| `mean` | `number` | ✅ | Mean value |

**Examples:**

```aql
CV(2.5, 50) // 5.0 (low variance)
CV(15, 50) // 30.0 (high variance)
COLLECT hour = DATE_TRUNC(d.timestamp, 'hour')
                   AGGREGATE avg = AVG(d.value), stddev = STDDEV(d.value)
                   LET cv = CV(stddev, avg)
                   RETURN {hour, cv, variance_level: cv < 5 ? 'low' : cv < 20 ? 'medium' : 'high'}
```

*Source: `retention_functions.h`*

---

### VARIANCE_LEVEL

**Signature:** `VARIANCE_LEVEL(cv, lowThreshold?, mediumThreshold?)` → `string`  

Classifies variance level based on CV thresholds

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `cv` | `number` | ✅ | Coefficient of Variation |
| `lowThreshold` | `number` | — | Low threshold (default: 5.0) |
| `mediumThreshold` | `number` | — | Medium threshold (default: 20.0) |

**Examples:**

```aql
VARIANCE_LEVEL(3.5) // 'low'
VARIANCE_LEVEL(15) // 'medium'
VARIANCE_LEVEL(25) // 'high'
VARIANCE_LEVEL(cv, 3, 15) // Custom thresholds
```

*Source: `retention_functions.h`*

---

## Type Functions

| Function | Description |
|----------|-------------|
| [IS_ARRAY](#is_array) | Checks if value is an array |
| [IS_BOOL](#is_bool) | Checks if value is a boolean |
| [IS_NULL](#is_null) | Checks if value is null |
| [IS_NUMBER](#is_number) | Checks if value is a number |
| [IS_OBJECT](#is_object) | Checks if value is an object |
| [IS_STRING](#is_string) | Checks if value is a string |
| [TO_ARRAY](#to_array) | Converts a value to an array |
| [TO_BOOL](#to_bool) | Converts a value to a boolean |
| [TO_NUMBER](#to_number) | Converts a value to a number |
| [TO_STRING](#to_string) | Converts a value to a string |
| [TYPENAME](#typename) | Returns the type name of a value |

### IS_ARRAY

**Signature:** `IS_ARRAY(value)` → `boolean`  

Checks if value is an array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to check |

**Examples:**

```aql
IS_ARRAY([1, 2]) // true
```

*Source: `document_functions.h`*

---

### IS_BOOL

**Signature:** `IS_BOOL(value)` → `boolean`  

Checks if value is a boolean

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to check |

**Examples:**

```aql
IS_BOOL(true) // true
```

*Source: `document_functions.h`*

---

### IS_NULL

**Signature:** `IS_NULL(value)` → `boolean`  

Checks if value is null

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to check |

**Examples:**

```aql
IS_NULL(null) // true
```

*Source: `document_functions.h`*

---

### IS_NUMBER

**Signature:** `IS_NUMBER(value)` → `boolean`  

Checks if value is a number

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to check |

**Examples:**

```aql
IS_NUMBER(123) // true
```

*Source: `document_functions.h`*

---

### IS_OBJECT

**Signature:** `IS_OBJECT(value)` → `boolean`  

Checks if value is an object

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to check |

**Examples:**

```aql
IS_OBJECT({a: 1}) // true
```

*Source: `document_functions.h`*

---

### IS_STRING

**Signature:** `IS_STRING(value)` → `boolean`  

Checks if value is a string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to check |

**Examples:**

```aql
IS_STRING("hello") // true
```

*Source: `document_functions.h`*

---

### TO_ARRAY

**Signature:** `TO_ARRAY(value)` → `array`  

Converts a value to an array

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to convert |

**Examples:**

```aql
TO_ARRAY("hello") // ["hello"]
TO_ARRAY([1, 2]) // [1, 2]
```

*Source: `document_functions.h`*

---

### TO_BOOL

**Signature:** `TO_BOOL(value)` → `boolean`  

Converts a value to a boolean

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to convert |

**Examples:**

```aql
TO_BOOL(1) // true
```

*Source: `document_functions.h`*

---

### TO_NUMBER

**Signature:** `TO_NUMBER(value)` → `number`  

Converts a value to a number

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to convert |

**Examples:**

```aql
TO_NUMBER("123") // 123
```

*Source: `document_functions.h`*

---

### TO_STRING

**Signature:** `TO_STRING(value)` → `string`  

Converts a value to a string

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Value to convert |

**Examples:**

```aql
TO_STRING(123) // "123"
```

*Source: `document_functions.h`*

---

### TYPENAME

**Signature:** `TYPENAME(value)` → `string`  

Returns the type name of a value

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `value` | `any` | ✅ | Any value |

**Examples:**

```aql
TYPENAME("hello") // "string"
TYPENAME(123) // "number"
TYPENAME([1,2]) // "array"
```

*Source: `document_functions.h`*

---
