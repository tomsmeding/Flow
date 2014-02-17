#Flow
Flow is a programming language designed to accomodate easy and smart operating on streamed data. Like `sed` operates on streams using regular expressions, Flow allows one to operate on streams using more advanced instructions. It is anticipated to become Turing-complete.

##Syntax

Flow uses the **loop block** as its main control structure. Loop blocks are delimited by braces, `{` and `}`, and contain code to be looped as prescribed by its condition, following the loop block in the code. Loop block and condition forms a **loop block construct**. For example, the following statement features one:

    {in + 1 ->> str;} until peek = newline;

Loop blocks contain statements which are executed as long as its condition prescribes. A condition using `until` prescribes that the code should be run while the following conditional yields a falsy value, and a condition using `while` prescribes running while it yields a truthy value. In the example case, the statement `in + 1 ->> str;` is run while the conditional `peek = newline` yields a falsy value.

A Flow **statement** consists either of a loop block construct as described above, or an assignment, and is terminated by a semicolon. An assignment takes the form `A->B` where the expression `A` is the thing assigned to `B`. If `B` is `out`, `A`'s result is outputted, else `B` is taken to be a variable name and `A`'s result is stored in that variable. If it is of the form `A->>B`, `A`'s result is _appended_ to the variable named `B`, creating it if nonexistent. Around the "arrow", whitespace is optional. After the arrow, there is always one token.

A Flow **expression** is in infix format using parentheses, which is the format you're all familiar with. This means that the expression `a + 1` just means "a plus one", `a + b * c` means "a plus the product of b and c" and `(a + b) * c` means "the product of c and the sum of a and b". Around all the symbols that are part of the language definition, which consist of one or more characters that are _not_ alphanumeric, whitespace is optional. This means that `a + 1` may also be written `a+1`, and taken together with the rules for the assignment arrow, the example statement could also be written:

    {in+1->>str;}until peek=newline;

A Flow **conditional** is simply an expression whose result is taken as a boolean. The comparison operators will be sighted more in conditionals than in other expressions though, but they _can_ be used everywhere. The **implicit boolean cast** performed is simple: everything is truthy in principle, except zero and an undefined value like NaN.

##Notation and literals

A **variable** name in Flow is valid if it is matched by the regular expression `^[A-Za-z_][A-Za-z0-9_]*$`: a letter or an underscore followed by any number of letters, underscores or numbers. Flow is case-sensitive.

Numbers are written normally, matched by the regular expression `^-?([0-9]+(\.[0-9]*)?|\.[0-9]+)$`. This means, `123`, `123.456`, `123.` (=`123.0`=`123`), `.123` (=`0.123`) and `-.1` are all valid, but `.` isn't and for example `123.456.789` is parsed as two numbers, `123.456` and `.789`.

Furthermore, strings are any sequence of characters surrounded by matching double quotes, `"like this"`. Inside a string literal, the sequences `\\`, `\"`, `\n`, `\r` and `\t` produce a literal backslash, a literal double quote, a newline, a carriage-return and a tab, respectively.

##Operators

Flow knows the following **operators**: _(lower precedence number means higher importance)_

Precedence|   Operator  | Meaning
:--------:|:-----------:|:-------
 &rarr; 0 |`(` `)`      | _Operator grouping, not really an operator. Works just like you expect._
 &rarr; 1 |`!`          | _prefix_(1 arg): Logical inverse of its argument
 &rarr; 2 |`*`          | _infix_(2 args): The product of its two operands
    2     |`/`          | _infix_(2 args): The quotient of its two operands
 &rarr; 3 |`+`          | _infix_(2 args): The sum of its two operands
    3     |`-`          | _infix_(2 args): The difference between its two operands
 &rarr; 4 |`>`          | _infix_(2 args): Whether its first operand is larger than its second
    4     |`<`          | _infix_(2 args): Whether its first operand is smaller than its second
    4     |`>=`         | _infix_(2 args): Whether its first operand is larger than or equal to its second
    4     |`<=`         | _infix_(2 args): Whether its first operand is smaller than or equal to its second
 &rarr; 5 |`=`          | _infix_(2 args): Whether its two operands are equal
    5     |`!=`         | _infix_(2 args): Whether its two operatnds are _not_ equal
 &rarr; 6 |`&&`         | _infix_(2 args): Yields true iff both its operands are truthy
    6     |`\|\|`       | _infix_(2 args): Yields true iff at least one of its operands is truthy
    6     |`^^`         | _infix_(2 args): Yields true iff exactly one its operands is truthy
    6     |`!&&`        | _infix_(2 args): Yields false iff both its operands are truthy
    6     |`!\|\|`      | _infix_(2 args): Yields true iff both its operands are falsy
    6     |`!^^`        | _infix_(2 args): Yields true iff its operands are both truthy or both falsy
 &rarr; 7 |`not`        | _prefix_(1 arg): =`!`
 &rarr; 8 |`and`        | _infix_(2 args): =`&&`
    8     |`or`         | _infix_(2 args): =`\|\|`
    8     |`xor`        | _infix_(2 args): =`^^`
    8     |`nand`       | _infix_(2 args): =`!&&`
    8     |`nor`        | _infix_(2 args): =`!\|\|`
    8     |`nxor`       | _infix_(2 args): =`!^^`

All associativity is left-to-right. Note<sub>1</sub> that `!` and `not` are **prefix**, not **infix** (which is only natural because this dictates a notation like `not a and b`{=`!(a&&b)`} or `!a or b`{=`(not a)||b`}). Note<sub>2</sub> also that all the boolean operators, those with precedence **8**, have the same precedence, and as such they are evaluated left-to-right, contrary to for example in C++ in which `&&` goes before `||`. Note<sub>3</sub> furthermore that if an operator has a symbol equivalent, that one always has a higher precedence by some amount.

There are two special **operators** that do not have operands:

   Operator  | Meaning
:-----------:|:-------
`in`         |Yields a character read from standard input
`peek`       |Works like `in` except that it doesn't consume the character: a subsequent read will get the same character

There are also a number of **constants** in Flow. They are in essence operators without operands that always return the same value. The following are built in:

Constant|Value
:------:|:----
`pi`|The ratio between the circumference and the diameter of a circle
`newline`|A single-character string containing ASCII 10 (LF)
`tab`|A single-character string containing ASCII 9 (HT)
`BEL`|A single-character string containing ASCII 7 (BEL)

No seperate boolean values exist in Flow, but the numbers `1` and `0` replace true and false, respectively, and due to the **implicit boolean cast** mentioned above, all numbers not `0` count as true.

---

_By Tom Smeding, 2013._

[1]: http://en.wikipedia.org/wiki/Reverse_Polish_notation