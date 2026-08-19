# The Archetype Language Reference Manual

*by Derek T. Jones*

## Overview

An Archetype program consists of a series of object declarations. These
declarations can be simplified by defining "types" so that the common
attributes of several slightly different objects can be described once; this is
known as "inheritance" in object-oriented parlance.

Objects consist of changeable attributes, which describe an object's
properties, and methods, which describe the object's behavior and how it
depends on its own attributes and its interaction with other objects. Methods
are tied to messages; when the object receives that message, the method
associated with it is invoked.

Methods consist of one or more statements tied to a message. To "invoke" a
method means to execute its statements in order from top to bottom. Some
statements have a value; the value of the last statement to be executed is
returned to the sender of the message that invoked the method.

An Archetype program is translated into an intermediate binary form and then
executed. Both jobs are done by the same program:

```shell
archetype --source=mygame.arch --include=games --create=mygame.acx
archetype --perform=mygame.acx
```

The interpreter contains a single object named `system`, which starts your
program by sending the message `'START'` to an object named `main`. For this
reason every Archetype program has to have one object named `main` which
contains a method for the `'START'` message.

## Object Declarations

An object declaration has the following form:

```
<type-name> ( <object-name> | null )

( <attribute-name> : <initial-value> )*

[ methods

( <message> : <statement> )* ]

end
```

A typical object declaration:

```
room bare_cell

  desc : "bleak, bare cell"

methods

  'look'  : write "I'm standing in a ", desc, "."
  'north' : hallway

end
```

In the above example, the type `room` had already been defined. A type must be
defined before it is used by an object. The only type that is already defined
for you is the `null` type. The `null` type means that the only attributes and
methods that the object contains are what appears in its particular
declaration. Whenever an object is declared to be of a certain type, that
object inherits the attributes and methods declared in the definition of the
type.

Please NOTE that when an attribute has a default expression, as above, the
attribute contains the *unevaluated* form of the expression. This is in
contrast to what happens when the attribute is assigned a new value at run time
with the `:=` operator (see [Operators and Expressions](#operators-and-expressions)
below). What this means is that in an object declaration such as:

```
null abc
  a   : 3
  def : a + 5
end
```

the value of the attribute `def` will change if the value of attribute `a`
changes.

When an object inherits an attribute or method, it means that unless that
object specifically redefines that attribute or method, it will have the
attribute or method defined by the type. Look at the example below:

```
type shouter based on null

  name : "an unremarkable shouting object"

methods

  'shout' : >>Shout, Shout, Ready Or Not!
  'who are you' :
    write "I am ", name, "."

end

shouter Bob

  name : "Bob"

end
```

The object Bob did not declare the method `'shout'` or the method
`'who are you'`, but inherited them from the `shouter` declaration because it
is a `shouter` type. It did not inherit the `name` attribute because it had its
own specific declaration for that. The code above could be replaced by the
single declaration below:

```
null Bob

  name : "Bob"

methods

  'shout' : >>Shout, Shout, Ready Or Not!
  'who are you' :
    write "I am ", name, "."

end
```

For a single object, the code above seems smaller and simpler, and perhaps it
is. But if you had even two `shouter` objects, using a common type definition
saves you time, makes the code clearer, and actually uses less memory while
being executed.

Type definitions can inherit attributes and methods from other type
definitions. In the above example, type `shouter` was based on `null`, meaning
that there was no inheritance. But we could define a type `screamer` based on
`shouter`:

```
type screamer based on shouter

  IsAscreamer : TRUE

methods

  'scream' : >>I CAN SCREAM TOO!

end
```

Now if we make an object of that type:

```
screamer Joe

  name : "Joe"

methods

  'sing' : >>Tra la la!

end
```

it is the same as if we had declared Joe as:

```
null Joe

  IsAscreamer : TRUE
  name : "Joe"

methods

  'sing' : >>Tra la la!
  'scream' : >>I CAN SCREAM TOO!
  'shout' : >>Shout, Shout, Ready Or Not!
  'who are you' :
    write "I am ", name, "."

end
```

The object `Joe` inherits attributes and methods first from the `screamer`
type, and then, because `screamer` is based on `shouter`, from the `shouter`
type.

The word `class` may be written in place of `type`; the two are exactly
synonymous, and much of the standard library uses `class`.

The trick of the `IsAscreamer` attribute is a common one in Archetype. If every
type declares an attribute named `IsA<type-name>`, it is easy to tell whether
an object's particular family tree has that type in it:

```
if any_object.IsAscreamer then
  write any_object.desc, " is a screamer."
```

Note that only `screamer` declares such an attribute above; `shouter` does not,
so `any_object.IsAshouter` would be UNDEFINED for everything, Joe included. The
convention only pays off if every type in the family follows it.

## Statements

An Archetype method is a single statement. However, this statement can be
compound, that is, made up of several statements inside curly braces `{ }`. For
example:

```
null obj

methods

  'ex1' : write "This is a single statement."
  'ex2' : {
    write "This is still a single statement because"
    write "both these statements are inside curly braces."
    }

  'ex3' :
    write "Although this might look like a compound statement,"
    write "it is not; this line and the next will be flagged"
    write "as errors when the program is compiled."

end
```

The `'ex3'` method is only tied to the first statement. The next two statements
are "stranded" and will prevent the program from being compiled into an
executable form.

## Reference To Archetype Statements

**How to read this section.** Each statement is preceded by a single line
giving its complete syntax in Backus-Naur form. If you are not familiar with
Backus-Naur form, read [Appendix A](#appendix-a-backus-naur-form).

### include

```
include "filename"
```

This is probably the first Archetype statement you will use, in order to
include `standard.arch`. The filename must be a string literal enclosed in
double quotes, and is written without its `.arch` extension, which is implied.
You can put a pathname on the include file if you like.

The compiler searches the current directory, then each directory given by the
`--include` option (a colon-separated list), then each directory named by the
`ARCHETYPE_INCLUDE` environment variable. All of the text in the file is
compiled as though you had included it there with your editor. A file that has
already been included is not included a second time.

### write, writes, write_centered, display, stop

```
( write | writes | write_centered | display | stop ) [ <expression> (, <expression> )* ]
```

These statements all take zero or more comma-separated expressions and write
them to the screen.

| Statement | What it does |
|---|---|
| `write` | Writes its expressions and finishes off the line. |
| `writes` | Keeps the line "dangling" so that subsequent output appears on the same line. |
| `write_centered` | Centers its output within the width of the screen. |
| `display` | Writes each expression's *diagnostic* form in brackets before its text, so that `display "x=", n` shows both the attribute named and the value it holds. For debugging. |
| `stop` | Halts the Archetype program after writing its output. |

All Archetype output is word-wrapped and paged. No words will be split across
the right margin, and text will not flow off the top of the screen before the
user can read it all. When the screen is full, a `(more)...` message is
displayed and the rest of the text is shown after a key is hit. The pager uses
the terminal's real height and width. In other words, all text output is
"civilized" without the programmer having to program specifically for it.

Examples:

```
drinkable poison
  desc   : "golden goblet"
  filled : TRUE
methods
  'look' : {
    writes "I look inside the ", desc, " and see "
    if filled then
      write "A frothing, noxious potion!"
    else
      write "nothing."
    }
  'drink' :
    if filled then
      stop "I drink it down... Garg!  I die in agony."
    else
      write "The ", desc, " is empty."
end
```

**Returns:** The write statements always return the value of the last
`<expression>` they write.

### The quoted paragraph (>>)

```
>> <any text to the end of the line>
```

A line beginning with `>>` writes everything after it, up to the end of the
line, exactly as the `write` statement would. For blocks of text that need a
particular kind of indentation it is more convenient and more readable than
`write`, because what you see is what you get. And since `>>` looks only for
the end of the line, any characters can follow it — including quotation marks
and commas — without prematurely ending the expression.

Consecutive `>>` lines are gathered into a *single paragraph* and wrapped
together. This is deliberate, and is what makes `>>` suitable for prose:

```
  'LONGDESC' : {
    >>The hallway runs north and south.  Someone has scratched a message
    >>into the plaster at eye level, but the light is too poor to read it.
    }
```

writes one wrapped paragraph, not two lines. Any other statement between them
ends the paragraph. If you want two `>>` paragraphs in a row, separate them
with a conditional or another statement.

### if

```
if <expression> then <statement> [ else <statement> ]
```

The `if` statement is extremely important because it is one of the only ways of
modifying an object's behavior based on any of its attributes. `<expression>`
is evaluated; if it is UNDEFINED, ABSENT, or FALSE, the statement following
`else` is executed; otherwise the statement following `then` is executed. If
`<expression>` is UNDEFINED, ABSENT, or FALSE and there is no `else` branch,
nothing happens. There must always be a `then` branch.

The `<statement>` following `then` or `else` can be another `if` statement. One
thing to be careful of when having another `if` statement follow a `then` is
that the next `else` encountered will be considered part of the most recent
`then`. If this is not what you want, use curly braces to surround the inner
`if` statement.

Example:

```
object airlock

  open      : TRUE
  locked    : TRUE

methods

  'leave' :
    if not spacesuit.wearing then
      stop "I'm not wearing a spacesuit!  I die."
    else if not spacesuit.patched then
      stop "All the air escapes through the hole in the suit!"
    else if not helmet.wearing then
      stop "I'm not wearing a helmet!  I die."
    else {
      write "I step out of the airlock."
      message --> object
      }

end
```

**Returns:** The value of the last executed `<statement>`. If there is no
`else` branch, and `<expression>` is UNDEFINED, ABSENT, or FALSE, then the
value of the `if` statement is UNDEFINED.

### case

```
case <test expression> of {
    ( <expression> : <statement> )* [ default : <statement> ] }
```

The `case` statement is a way of checking an expression to see if it matches
one of several values. `<test expression>` is evaluated only once. Then the
`<expression> : <statement>` pairs following the word `of` are evaluated from
first to last until one of the `<expression>`s matches `<test expression>`. If
this happens, the `<statement>` following the colon is executed. The word
`default` matches any `<test expression>` and therefore must be at the end of
the `case` statement. The `case` statement usually replaces a more complicated
if-then-else-if structure.

Instead of

```
{ tries +:= 1
  if tries = 1 then
    write "I try to open it with my bare hands, but I can't."
  else if tries = 2 then
    write "I try and try but it stays closed."
  else if tries = 3 then
    write "I scream with frustration, but the can won't open."
  else if tries = 4 then
    write "I pound the can upon the pavement.  Stays shut."
  else
    write "No.  I've given up." }
```

you can write

```
  case tries +:= 1 of {
    1 : write "I try to open it with my bare hands, but I can't."
    2 : write "I try and try but it stays closed."
    3 : write "I scream with frustration, but the can ",
              "won't open."
    4 : write "I pound the can upon the pavement.  Stays shut."
    default : write "No.  I've given up."
    }
```

### while

```
while <expression> do <statement>
```

The `while` statement is the most straightforward way to do something
repeatedly. `<expression>` is evaluated; if it is UNDEFINED, ABSENT, or FALSE,
the `while` statement ends. If not, `<statement>` is executed and
`<expression>` is evaluated again.

There are two ways to stop a `while` loop: `<statement>` must eventually cause
`<expression>` to evaluate to UNDEFINED, ABSENT, or FALSE, or `<statement>`
must contain the `break` statement.

Note that if `<expression>` is UNDEFINED, ABSENT, or FALSE before entering the
`while` loop, `<statement>` will never be evaluated.

Examples:

```
{               # sums up the numbers between 1 and x
  while x > 0 do {
    sum +:= x
    x -:= 1
    }
  write "The sum is ", sum
}
```

```
# interactive questioning

  while TRUE do {
    writes "What message would you like to send? "
    if (m := read) = "quit" then
      break
    else
      m -> obj
  }
```

Note that in the above example, the only way the loop can exit is with the
`break` statement, since `<expression>` will always be TRUE.

### for

```
for <expression> do <statement>
```

The `for` statement is the other way of doing things repeatedly. It is used
when you want to do some action to all or some set of the objects in your
program.

`<expression>` should contain the keyword `each`. This keyword is set, in turn,
to each object in your program. Every time `<expression>` does not evaluate to
UNDEFINED, ABSENT, or FALSE, `<statement>` is executed. `<statement>` will
usually contain the keyword `each` as well, which retains its value until
`<expression>` is evaluated again.

```
object purse

methods
  'empty' : {
    write "I empty out the purse and find:"
    for each.location = purse do {
      write each.desc
      each.location := player.location; 'MOVE' -> each
    }
  }
end
```

If you want `<statement>` to apply to every single object in your program, not
just those that satisfy some `<expression>`, write

```
for each do ...
```

Note that even if `<expression>` only specifies a few objects out of the
hundred or so in your program, all of the program's objects must be tested.

If you want to find just the first instance of the set of objects you're
looking for, you can use the `break` statement to jump out the instant
`<statement>` is evaluated:

```
# player needs a container in their possession
{
  useful := UNDEFINED
  for each.IsAcontainer and each.location = player do {
    useful := each
    break
  }
  if useful then
    write "You can use ", useful.desc, "."
  else
    write "You don't have anything useful."
}
```

### break

```
break
```

Leaves the innermost enclosing `while` or `for` loop immediately.

Outside of a loop, `break` is not diagnosed as an error. It unwinds until
something catches it, which means a stray `break` in a method silently
abandons not only the rest of that method but the statement that sent the
message. Use it only inside a loop.

### create

```
create <type name> named <attribute>
```

Most of the objects in your program will probably be defined within the program
itself. However, there are some situations where you would like to add an
object to your program dynamically, that is, after the program is already
underway. The `create` statement is the way to do this.

This statement instantiates an object of `<type name>` and causes
`<attribute>` to point to it. `<attribute>` is said to *reference* the new
object. The new object will not have its own name; it is a nameless object. If
you assign some other value to `<attribute>` without keeping track of the new
object some other way, it will be hard to find again. (The `for` statement will
pick it up, however.)

Here is one application. The following types define a list. The nodes act like
coathangers: the senders of the `'Attach Me'` messages are pointed to by the
nodes, which in turn point to the next node down the list. In order for this to
work, the nodes have to be created as they are needed.

```
type node based on null

  next  : UNDEFINED
  refer : UNDEFINED

end

type list based on null

  head : UNDEFINED
  temp : UNDEFINED

methods

  'Attach Me' : {
    create node named temp
    temp.refer := sender
    temp.next := head
    head := temp
    }

  'Display List' : {
    temp := head
    while temp do {
      'Display Self' -> temp.refer
      temp := temp.next
      }
    }

end
```

Note that the example above assumes that the senders of the `'Attach Me'`
message have a `'Display Self'` method which they can respond to.

Since version 4.0 there is also a native list type, and the linked list above
can be written as a single attribute; see [Lists](#lists) under Data Types.
The `create` statement remains the way to make new *objects* at run time —
things with attributes and methods of their own.

### destroy

```
destroy <attribute>
```

This statement performs the opposite of the `create` statement. If
`<attribute>` is pointing to a dynamically instantiated object, the object will
be destroyed. We can extend the `list` type defined above by adding the
following method:

```
'Shrink List' : {
  if head then {
    temp := head
    head := head.next
    destroy temp
    }
  }
```

This method shrinks the list by one element by removing the top element.

### keyword

```
keyword <identifier> (, <identifier> )*
```

Declares one or more identifiers as keywords — state values, described under
[Data Types](#keywords-state-values) below. Declaring them is optional, since any
identifier that is not an object, type, or attribute is treated as a keyword
anyway. Declaring them explicitly helps catch spelling errors that might
otherwise be very difficult to debug, particularly in a large program.

## Keywords

Archetype has a number of keywords which cannot be used as object, type, or
attribute names because they evaluate to special values depending on their
context.

| Keyword | Meaning |
|---|---|
| `each` | UNDEFINED outside of a `for` loop. Within a `for` loop, evaluates to each object in the program in turn. |
| `key` | Evaluates to a single keypress. Execution stops until the user presses a key. |
| `message` | The last message received by the object. Most useful in a default method, where you may not know at the time of writing which message might be invoking the method. |
| `read` | Like `key`, except that execution is suspended until the user terminates their input with a RETURN. `read` then evaluates to the entire line the user typed. |
| `self` | The current object. Most useful in a type definition, where you are trying to code for the general case. |
| `sender` | The sender of `message`. Whenever the data a method needs *is* the sender, this is the channel it arrives on. |

## Operators and Expressions

There are exactly three forms of an expression, which can be represented by the
following BNF:

```
<expression> ::= ( <value> | ( <expression> <binary operator> <expression> ) | ( <unary operator> <expression> ) )
```

The operators following are discussed in rough order of their importance and
significance.

### Assignment ( `:=` `+:=` `-:=` `*:=` `/:=` `&:=` )

The assignment operator always has the form `<attribute> := <expression>`. If
the left side does not evaluate to an existing attribute, a warning will be
given. If it does, then the value of `<expression>` is placed into
`<attribute>`. The former value of `<attribute>` is destroyed.

The assignment operator has several cumulative forms: `+:=`, `-:=`, `*:=`,
`/:=`, and `&:=`. These are all of the form `<operator>:=` and have the same
effect as the statement `<attribute> := <attribute> <operator> <expression>`,
but are shorter and clearer. In other words, instead of

```
a := a + 1
```

write

```
a +:= 1
```

The value of the expression is always the final value of `<attribute>`.
Assignment operators group right to left, which is unusual (most Archetype
operators group left to right); this is so that successive assignments can be
made to the same value, so that

```
subj := dobj := verb := prep := UNDEFINED
```

will set all four attributes to UNDEFINED.

NOTE: it is important to remember that the attribute on the left hand side of
the assignment receives the *value* of the expression on the right, not its
unevaluated form. A statement such as `abc := main.dobj` sets `abc` to the
current value of `main.dobj`. If `main.dobj` changes later, `abc` will not.
This is in contrast to the practice of initializing an attribute with an
expression, as described above in [Object Declarations](#object-declarations).

Beware that `s &:= expr` leaves `s` UNDEFINED if `expr` is UNDEFINED, rather
than leaving `s` alone.

### Sending Messages ( `->` `-->` `<-` )

These operators are of the form `<message> -> <object>` or
`<message> -> <type name>`, and simply send `<message>` to `<object>`. If
`<object>` has a method defined for `<message>`, that method will be invoked,
and the value of the expression will be the value of the last statement
executed in the method. If `<message>` is not a defined message, or `<object>`
is not a defined object, the value of the expression is UNDEFINED and the
message is never sent.

Note that the vocabulary of sendable messages is built from all the
single-quoted literals in your program. This simply means that you should
always put messages in single quotes.

If `<message>` and `<object>` are both valid, but `<object>` does not have a
method defined for `<message>`, the value of the send expression is the word
ABSENT. Note that this is not just a system-generated message; an object can
actually return ABSENT if it wishes to inform the sender that it did not handle
the message. This is most commonly used if an object has defined a default
method, since the presence of such a method means the object automatically
handles any message given it. For example, the following default method handles
direction messages, but not any others:

```
default :
  if message -> compass then
    message -> handler
  else
    ABSENT
```

The **pass** (`-->`) operator invokes the appropriate method from `<object>`,
but executes the method within the context of the *current* object, as though
the current object were receiving the message. This is always the functionality
used whenever the receiver is a `<type name>`, since types do not have
attributes.

One common reason to pass a message to a type is when you are defining an
extension to an existing method in your type, and you do not wish to type (or
do not know) all the previous code:

```
object superglue

  location : drawer
  desc     : "super glue"
  sticky   : TRUE

methods

  'drop' :
    if location = player and sticky then
      write "I can't!  It's stuck to my fingers!"
    else
      message --> object                # do what you normally do

  'dissolve' :
    if sticky then {
      write "I dissolved the super glue."
      sticky := FALSE
      }
    else
      write "It's already dissolved."

end
```

The **send-to** (`<-`) operator is `->` written the other way around:
`<object> <- <message>` sends `<message>` to `<object>`, and evaluates to the
*recipient* rather than to the reply. Because it yields the recipient, sends
chain, and they arrive in written order:

```
system <- 'INIT SORTER' <- "dog" <- "Ajax" <- "cat" <- 'CLOSE SORTER'
```

This is worth having because some protocols are a sequence of messages sent for
effect, where the interesting value is not any one reply. It also lets a
recipient be primed in place, so that the argument's reply can be used in the
same expression:

```
"grab" -> (system <- 'WHICH OBJECT')
```

Prefer `<-` when you are setting the table for the send that follows. When you
care about the answer, a list message (see
[Advanced Topics](#advanced-topics-messages-with-arguments)) says the same
thing in one send and keeps the reply.

### Conditionals ( `=` `~=` `>` `<` `>=` `<=` )

These operators compare two values and return either TRUE or FALSE, if their
operands are comparable, or UNDEFINED if not. They are used almost exclusively
within `if`, `while`, and `for` statements, although they will always return
their value no matter where they are used.

There is no `<>`; the not-equal operator is `~=`.

If both operands are or can be converted to numbers, they are compared
numerically. If not, then they are compared as strings, if possible. If they
cannot be converted to strings, then the quantitative comparisons
(`<`, `>`, `<=`, `>=`) will return UNDEFINED.

The equal (`=`) and unequal (`~=`) operators are special because they can
compare non-numeric, non-string values to simply see if they are identical or
not. Even though the operators `<=` and `>=` mean "less than or equal" and
"greater than or equal", these cannot be used in all the same places, because
they are still checking quantitative equality. A few examples will make things
clearer:

| Comparison | Result |
|---|---|
| `5 < 6` | TRUE because five is less than six. |
| `"mosquito" < "moth"` | TRUE because "mosquito" comes before "moth" in the ASCII alphabet. |
| `5 < "four"` | TRUE. Since "four" cannot be converted to a number, 5 is converted to the string "5", and numbers come before letters in the ASCII alphabet. |
| `5 < "4"` | FALSE. "4" can be converted to the number 4; they are compared as such and 5 is greater than 4. |
| `"abc" = "abc"` | TRUE. |
| `"def" ~= "abc"` | TRUE. "def" is not "abc". |
| `"abcd" = "abc"` | FALSE. Two strings must be the same character for character. |
| `player.location = ballroom` | TRUE if the `location` attribute of the player object points to the ballroom object. |
| `7 <= 7` | TRUE. |
| `player.location <= ballroom` | UNDEFINED. There is no universal sense in which object references can be measured. |
| `UNDEFINED < 7` | UNDEFINED. UNDEFINED has no measurable value in any sense. |
| `UNDEFINED = 7` | FALSE. 7 is indeed defined: as 7. |
| `box.wearing = UNDEFINED` | TRUE if either the `wearing` attribute of the box has not been defined or if it has been explicitly assigned UNDEFINED. |
| `[1 2 3] = [1 2 3]` | TRUE. Lists compare by structure, element for element. |
| `[1 2 3] = "[1 2 3]"` | FALSE. A list is not its own printed form. |
| `[1 2] < [1 3]` | UNDEFINED. Lists have no ordering. |

### The Logical Operators ( `and` `or` `not` )

These operators always return values of TRUE and FALSE. They are used within
`if`, `while`, and `for` statements (although they will return their values in
other contexts as well) to take action based on a number of conditions:

- `and` is TRUE if neither of its operands is FALSE, ABSENT, or UNDEFINED.
- `or` is TRUE so long as at least one operand is neither FALSE, ABSENT, nor UNDEFINED.
- `not` is unary; it returns TRUE if its operand is FALSE, ABSENT, or UNDEFINED, and FALSE if not.

Because the precedences of these operators are all beneath those of the
comparison operators, you can form natural expressions without parentheses such
as:

```
if player.location = trapdoor_room and trapdoor.is_closed then
```

If you ever get confused, however, use parentheses to ensure correctness. They
will not take any more memory at run time. However, familiarity with the
operators' precedence is best of all, and the fewer parentheses, the more
readable your code.

### The Arithmetic Operators ( `+` `-` `*` `/` `^` )

These perform simple arithmetic on numbers or values that can be converted to
numbers, returning the result.

| | |
|---|---|
| `^` | Exponentiation. Raises its left operand to the power of its right. Performed right to left. |
| `*` `/` | Multiplication and division. Performed left to right. |
| `+` `-` | Addition and subtraction. Performed left to right. |

This is the normal algebraic precedence of operators. Thus:

```
3 + 5 * 2 ^ 2 = 23
```

If you intend another precedence, simply use parentheses:

```
((3 + 5) * 2)^2 = 256
```

### The String Operators ( `&` `within` `length` `leftfrom` `rightfrom` )

| | |
|---|---|
| `&` | Concatenation. Both operands are converted to strings; the value is UNDEFINED if this is not possible. `"the " & "cupboard"` becomes `"the cupboard"`. |
| `within` | Search. Returns an integer giving the position in the second string where the first string can be found. `"boar" within "cupboard"` = 4. If the first string cannot be found in the second, the value is 0. UNDEFINED if either operand is a list. |
| `length` | A unary operator. On a string, returns the number of characters: `length "snake"` = 5, `length ""` = 0. On a list, returns the number of elements. UNDEFINED otherwise. |
| `leftfrom` | Its left operand is a string; its right operand is the position in the string to take characters to the left from. `"cupboard" leftfrom 4` = `"cupb"`, `"cupboard" leftfrom 10` = `"cupboard"`. |
| `rightfrom` | Its left operand is a string; its right operand is the position to take characters to the right from. `"cupboard" rightfrom 4` = `"board"`, `"cupboard" rightfrom 1` = `"cupboard"`. |

The `leftfrom` and `rightfrom` operators are a little clumsy; character range
specification would certainly be clearer. However, substring operations are not
all that common in adventure game applications. The real reason is that the
design I chose for my expression parser couldn't handle range specifications.

The string operators that perform *text surgery* — `within`, `leftfrom`,
`rightfrom` — are UNDEFINED when handed a list, rather than quietly operating
on the list's printed form. The rule is that a coercion answers the question
"how do you read as text?", while an operator needs a *test* only when its
meaning depends on what the value is made of. So `&` still converts a list to
its text, and `length` counts what is actually there.

### The List Operators ( `@` `head` `tail` )

A list is written between square brackets, its elements separated by spaces —
not commas:

```
[ "north" "south" "east" "west" ]
```

Each element is a full expression, so `[ 'PUSH' sender.pronoun read ]` is a
list of three.

| | |
|---|---|
| `@` | Pair. Joins its left operand onto the front of its right. `1 @ [2 3]` is `[1 2 3]`. |
| `head` | The first element. `head [1 2 3]` = 1. |
| `tail` | Everything but the first element. `tail [1 2 3]` = `[2 3]`. |

A list is built from pairs, so `[1 2 3]` is exactly `1 @ (2 @ (3 @ UNDEFINED))`.
An empty list terminates as UNDEFINED, which is why `[]` and UNDEFINED are the
same value, and why `length []` is UNDEFINED rather than 0: an attribute nobody
ever set reads as UNDEFINED too, and answering 0 would claim that a missing
attribute holds zero things.

**Every value is a list of at least itself.** `head` of anything that is not a
list is that thing: `head 5` is 5, and `head [5]` and `head 5` are the same
value. The `tail` is what tells an atom from a list of one — the tail of an
atom is UNDEFINED, deliberately, because otherwise every loop that walks a list
would never find the end of it. The usual way to walk a list is therefore:

```
node := lst
while node do {
  'Consider' -> head node
  node := tail node
  }
```

A pair whose tail is not itself a list is *improper*, and prints differently to
show it: `1 @ 2` writes as `(1 @ 2)`, where `[1 2]` writes as `[1 2]`. `length`
counts an improper tail, so `length (1 @ 2)` is 2.

The `@` operator binds *loosely* — more loosely than comparison — so that
whatever computes an element finishes before the element is joined on, and
`a + 1 @ rest` is a list rather than an error. The consequence to watch for is
that a comparison against a list built with `@` needs parentheses:

```
if x = (1 @ rest) then ...        # not  x = 1 @ rest
```

### The Conversion Operators ( `chs` `numeric` `string` )

These operators are unary and return some transformation of their operand. They
are UNDEFINED if their operand cannot be thus converted.

| | |
|---|---|
| `chs` | CHange Sign. For numbers only; inverts the sign. Its alias is a minus sign in front of a number, so `(5 * -3)` and `(5 * chs 3)` are equivalent internally. |
| `numeric` | Converts a string explicitly to a number. Its alias is a plus sign in front of the string, so `(numeric "453")` and `(+"453")` are equivalent internally. |
| `string` | Converts its operand to a string if possible. TRUE becomes `"TRUE"`, 364 becomes `"364"`, `[1 2 3]` becomes `"[1 2 3]"`. Its alias is an ampersand in front of the value, so `(string 453)` and `(&453)` are equivalent internally. |

Because all Archetype operators attempt to convert their operands to the
necessary type anyway, these operators are largely unnecessary except to test
whether the conversion is possible, as in

```
if numeric obj.attr then ...
```

### The Random Operator ( `?` )

This operator is unary and simply returns a random number in the range
1 to `<operand>`. To simulate two six-sided dice rolling, for example, you
might write

```
diceroll := ?6 + ?6
```

but not

```
diceroll := ?6 * 2
```

which would have the effect of rolling a single die and then doubling its
value.

Runs can be made repeatable by giving the interpreter `--seed=N`.

### The Dot Operator ( `.` )

This ubiquitous operator expects an object reference on its left and an
attribute name on its right. If the object has defined or inherited an
attribute by that name, it returns the value of that attribute. If not, it
returns UNDEFINED. The dot operator groups left to right (like most operators)
so that `player.location.location.open` refers to the `open` attribute of the
object pointed to by the `location` attribute of the object pointed to by the
`location` attribute of the `player` object. In other words, only the very
leftmost operand in such a string will be an object reference; the rest must be
attribute names.

## Precedence of Operators

Any expression enclosed in parentheses is evaluated first, beginning with the
innermost set of parentheses. Where parentheses are not used, operators with
the highest precedence are evaluated first.

| Operator | Precedence | Grouping |
|---|---|---|
| `.` | 13 | left to right |
| `chs` `numeric` `string` `?` `length` `head` `tail` | 12 | unary |
| `^` | 11 | right to left |
| `*` `/` | 10 | left to right |
| `+` `-` `&` | 9 | left to right |
| `within` | 8 | left to right |
| `leftfrom` `rightfrom` | 7 | left to right |
| `->` `-->` `<-` | 6 | left to right |
| `=` `~=` `>` `<` `>=` `<=` | 5 | left to right |
| `not` | 4 | unary |
| `and` | 3 | left to right |
| `or` `@` | 2 | left to right |
| `:=` `*:=` `/:=` `+:=` `-:=` `&:=` | 1 | right to left |

The unary selectors bind tightly and take a single operand, so `head lst = x`
compares the head rather than taking the head of a comparison.

## Data Types

### Strings

Strings are represented within double quote marks. If the string is supposed to
contain a double quote mark itself, precede the double quote with a backslash.
Doing so indicates that the double quote is to be part of the string, not the
end of it. For example,

```
write "My name is \"Bob\""
```

produces

```
My name is "Bob"
```

Two backslashes in a row become a single backslash within a string. There are
four other characters that, when preceded by a backslash, produce a special
character:

| | |
|---|---|
| `\b` | The backspace character |
| `\e` | The escape character (character 27) |
| `\t` | The tab character |
| `\n` | The newline |

The word-wrap and paging engine understands `\n` and starts a new line at it,
counting the line against the page.

Strings have no length limit. (The 256-character ceiling of the original
implementation was inherited from Turbo Pascal, and went away with it.)

### Numbers

Archetype supports 32-bit signed integers only. No real numbers, no
floating-point arithmetic. Numbers therefore have the range −2,147,483,648 to
2,147,483,647, and arithmetic that leaves that range wraps around silently.

### Lists

A list is a chain of pairs, written between square brackets with its elements
separated by spaces. Its operators are `@`, `head`, and `length`, described
under [The List Operators](#the-list-operators-head-tail-). An empty list is
UNDEFINED; every non-list value is the head of itself.

Lists can be converted to strings — `"items " & [1 2 3]` is `"items [1 2 3]"` —
but they compare structurally, so a list is never equal to its own printed
form.

### Messages

A message is enclosed in single quotes, as opposed to a string, which is
enclosed in double quotes. Any message can be converted to a string, but a
string can only be converted to a message if the message appears somewhere else
in your program in single quotes. The reason for this is that messages are
stored in the `.acx` file as a single number. This improves speed and memory
usage a great deal, and encourages the use of a message as a unique constant
instead of as a free-form string. Therefore you do not have to worry that a
long message such as `'DEBUG EXPRESSIONS'` will be slower to send and receive
than `'AB'`; it won't.

It is an Archetype convention to use all caps in a system message and all
lowercase if the message represents a word from the parser. If you define your
own messages for simple inter-object communication, the convention is to
capitalize the first letter, as in `'Repeated Action'`.

### Objects

These are declared in the source code or created with the `create` statement.
They cannot be converted to and from any other data type. They are valid on the
right hand sides of the `->`, `-->`, and `:=` operators, and on the left hand
side of the dot operator.

### Types

These are only declared within the source code; they cannot be dynamically
created. There is little you can do at run time with a type object: you are not
allowed to reference its attributes with the dot operator, and you cannot send
messages to it with `->`. You can, however, pass messages to it with `-->`. In
fact if you use `->` with a type on the right hand side, it will be demoted to
`-->`. This can be useful when an object has redefined a method but needs to
invoke the original.

### Keywords (state values)

Any identifier not declared as an object, type, or attribute is a keyword: a
state value. These can be assigned and compared against but have no meaningful
conversion to strings or numbers. They may be declared with the `keyword`
statement, which is worth doing in a large program because it catches spelling
errors that are otherwise very difficult to debug.

## The System Object

As mentioned earlier, there is a special object name, `main`, to which
Archetype sends the message `'START'` when the interpreter starts up. The
second special object name in Archetype is `system`, and refers to the system
object that is part of the interpreter. This object performs parsing and
parsing-related operations, sorting of lists of strings, debugging, banners, and
save and restore of the state of the interpreter.

The system object is also different in one major respect from all other objects
in Archetype: it is the only object that can receive free-form strings as
messages. All other Archetype objects can only receive strings which appear
elsewhere as messages.

Generally, the system object is sent a message which puts it into one state or
another, and then all strings it receives until another special string are
considered data. A list message performs the whole dance in one send; see
[Advanced Topics](#advanced-topics-messages-with-arguments) below.

### Messages Pertaining to Parsing

| Message | Meaning |
|---|---|
| `'INIT PARSER'` | Initializes the parser and then puts the system into its vocabulary-building state. See `'OPEN PARSER'`. |
| `'OPEN PARSER'` | Puts the system into vocabulary-building state without wiping out the vocabulary already built. In this state, all messages are assumed to be names of verbs or nouns, except `'VERB LIST'`, `'NOUN LIST'`, and `'CLOSE PARSER'`. The object which the name refers to is assumed to be the *sender*. Multiple names for a single object can be separated by vertical bars; the message `'rock|stone|rowlirag'` is three synonymous names for the sender. |
| `'VERB LIST'` | All subsequent messages are considered to be verb names or synonyms. |
| `'NOUN LIST'` | All subsequent messages are considered to be nouns. Either `'VERB LIST'` or `'NOUN LIST'` must be sent after an `'OPEN PARSER'`; there is no default. |
| `'CLOSE PARSER'` | Stop receiving vocabulary items; return to idle state. |
| `'ROLL CALL'` | To help it resolve ambiguities, the system needs to know which objects are near the player. If six objects are all called "button" and the player types "push button", the button indicated is probably the one nearby. `'ROLL CALL'` erases the current sense of what is nearby and prepares the system to receive `'PRESENT'` messages. |
| `'PRESENT'` | The sender is assumed to be a nearby object, and will continue to be considered such until the next `'ROLL CALL'`. |
| `'PLAYER CMD'` | Takes a single argument: the exact string the player typed in. |
| `'NORMALIZE'` | Returns the normalized player command as of the last `'PLAYER CMD'`. All lowercase, words separated by a single space, all punctuation removed except apostrophes and hyphens, always with one trailing space. If the player typed nothing at all, returns a single space. |
| `'PARSE'` | Parses the last string received through `'PLAYER CMD'`. Returns nothing. Gets rid of all instances of "a", "an", and "the" before parsing. |
| `'NEXT OBJECT'` | How the system indicates which objects matched what words. The player's string will have been turned into a sequence of objects — possible because even verbs have objects associated with them. Returns the next object in the list, left to right. If a word or phrase didn't translate, returns the string that failed to parse. When the list is exhausted, returns UNDEFINED, and does so until `'PARSE'` is sent again. |
| `'WHICH OBJECT'` | Looks up an object given its parse name. Takes a single argument, the name; returns the object with that name, or UNDEFINED if no such object exists. The lookup is based on the vocabulary given to the system since the last `'CLOSE PARSER'`. |

### Messages Pertaining to Sorting

| Message | Meaning |
|---|---|
| `'INIT SORTER'` | Initializes the system object's sorting algorithm, then opens the sorter as if `'OPEN SORTER'` had been sent. |
| `'OPEN SORTER'` | Prepares the system object to receive data. Every string sent to the system is dropped on the heap as sortable data until `'CLOSE SORTER'`. |
| `'CLOSE SORTER'` | Stop receiving sort data; return to idle state. |
| `'NEXT SORTED'` | Returns the next string in a sorted list. Sorting is always ascending, so the string returned is the lexically smallest remaining. After being returned it is removed from the heap, and when the heap is empty, `'NEXT SORTED'` returns UNDEFINED. |

### Messages Pertaining to Presentation

| Message | Meaning |
|---|---|
| `'BANNER'` | Takes a single argument, a one-character string, and rules a line across the full width of the screen with it. `['BANNER' '-'] -> system` is what puts the rules above and below a room description. |

### Messages Pertaining to Debugging

These three messages are toggles: sending the message turns the associated
property on, and sending it again turns it off.

| Message | Meaning |
|---|---|
| `'DEBUG MESSAGES'` | Prints every message that is sent, who it is being sent to, and who is sending it. |
| `'DEBUG EXPRESSIONS'` | Prints every expression that is going to be evaluated and what it evaluates to. |
| `'DEBUG STATEMENTS'` | Prints every statement that is going to be executed. |

The `display` statement and the `--repl` and `--inspect` options of the
interpreter cover most of what these were for.

### Messages Pertaining to the Interpreter State

Both of these messages take a single argument, the name of the file to use as
the state file, and return UNDEFINED if the file cannot be opened.

| Message | Meaning |
|---|---|
| `'SAVE STATE'` | Saves the state of the interpreter to the given file name. This includes the values of all attributes, any dynamically created objects, and the assembled vocabulary. Returns TRUE when done. |
| `'LOAD STATE'` | Loads all of the information in the state file with the given file name, replacing the current universe entirely. Returns TRUE when done. |

A save file is a mutated copy of the whole compiled program, not a patch
against it, which is what makes an `.acx` fully resumable. The original
restriction — that a state file could only be loaded by the exact `.acx` that
wrote it — no longer applies, because the state file *is* an `.acx`.

## Advanced Topics: Messages With Arguments

Everything above describes a language in which a message carries no data of its
own. That was a deliberate experiment, and it ran a long time: a method can
always tell who the sender was and read the sender's attributes, and for a
great many purposes that is enough. When the argument *is* the sender —
`'ADD SELF' -> location` — `sender` is the right channel and always was.

Since version 4.0 a message may also be a list, in which case the **head names
the method and the rest rides along as arguments**:

```
['MOVE TO' player] -> vase
```

There is no new syntax at the send site; a list literal is a list literal. The
rule is simply that dispatch always goes by the head, and since `head` of any
non-list value is that value, a bare message is a list of one and there is no
second case.

Inside the method, `message` remains bound to the **whole list**, not to the
head. This is what makes forwarding free:

```
  'MOVE TO' :
    if dest.locked then
      write "It won't fit through."
    else
      message --> object          # arguments intact, nothing unpacked
```

The arguments are read with `tail message`, and the method's own name — useful
in a default method, which now sees list and bare messages alike — with
`head message`:

```
  default :
    case head message of {
      'MOVE TO' : ...
      default   : ABSENT
      }
```

### What arguments are for, and what they are not for

The honest use for an argument is a value that is *neither the sender nor
already visible to the receiver*. Three cases where that is true:

- **A precondition about a third party.** A method that must decide something
  about a destination has to be handed the destination, and `sender` is already
  spoken for by the thing that wants to move.
- **A mode.** `['DESCRIBE' 'BRIEF']`.
- **A one-shot to `system`**, described below.

What arguments are *not* for is repeating what `sender` already says.
`['ACCEPT' self] -> location` spends a list to say what `'ACCEPT' -> location`
says for free. And a refusal that does not depend on any argument wants no
argument at all: make it a method that is ABSENT for everything without an
objection, since ABSENT falls through an `if` on its own, and only the
exceptions have to write anything.

### List messages to `system`

The system object's protocols are sequences of messages, and a list message to
`system` is that whole sequence in a single send: the head first, then each
element of the tail in order, with the reply of the whole being the reply of
the last. So a lookup that used to take two statements takes one, and keeps its
answer:

```
if match := ['WHICH OBJECT' verb] -> system then ...
```

A control message rides in a tail like any other element, so an entire
open-feed-close protocol fits in one send:

```
['INIT SORTER' "b" "a" 'CLOSE SORTER'] -> system
```

And because the send is atomic, `system` can never be caught between states —
which matters most for the two messages that can be interrupted by the state of
the world changing underneath them:

```
if ['SAVE STATE' read] -> system then
  write "Game saved."
```

The staged form is still the right one when there are genuinely many sends: when
each send's *sender* carries meaning, as when objects announce their own names
to the parser, or when the data comes from a `for` loop, which a fixed-length
list literal cannot express.

## Appendix A: Backus-Naur Form

The Backus-Naur Form (or BNF) is a way of representing syntax. In this manual,
the meaning of the various symbols is as follows:

| | |
|---|---|
| **boldface** | Should be typed literally. |
| `<words>` | Something in angle brackets is symbolic; it refers to something else. |
| `( )` | Parentheses mean that everything inside is to be considered one thing. |
| <code>&#124;</code> | The vertical bar separates alternatives. `(a｜b｜c)` means exactly one of a, b, or c. `(a (b｜c))` means ab or ac. |
| `[ ]` | Square brackets mean that what is inside is optional. `(a [b｜c])` means a, ab, or ac. |
| `*` | The asterisk means "zero or more" of what it follows. `ab*` means a, ab, abb, abbb, and so on. |

Example. The following BNF:

```
( write | writes | stop ) [ <expression> (, <expression> )* ]
```

can be expressed in English as: either the word `write`, `writes`, or `stop`,
optionally followed by an expression and zero or more instances of a comma
followed by another expression.

Some of the valid statements you can construct from this BNF, leaving
`<expression>` within angle brackets, are:

```
write
stop <expression>
writes <expression>, <expression>
write <expression>, <expression>, <expression>
stop
```
