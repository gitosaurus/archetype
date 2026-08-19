# How To Quickly Write An Adventure Game

*by Derek T. Jones*

This document describes how to write your own adventure game quickly and
easily, and also how to take advantage of more advanced features when your game
becomes more complex. You should already be familiar with what playing an
adventure game is like. If you aren't, try the adventures that came with this
package to understand the feel of moving through an artificial universe by text
and imagination alone.

Archetype is more than just a language for writing adventures. It is a more
general-purpose language that was designed with the writing of adventure games
in mind. For this reason, there is a good deal of existing Archetype code that
must be combined with your own adventure's objects and rooms in order for them
to behave like a text adventure. Here's an example of the simplest adventure
possible:

```
include "standard"

room first_room

methods

  'INITIAL' : 'START HERE' -> player

end
```

Try it. Type that into a text file and name it `test1.arch`. Any text editor
will do; what matters is that the file is plain text, with no formatting of its
own.

Now create the adventure. Before it can be played, it has to be translated into
a binary format so that it can be loaded and executed quickly. During this
translation the program you wrote is also checked for any syntax errors,
meaning errors in Archetype grammar — not English grammar!

```shell
archetype --source=test1.arch --include=games --create
```

That writes `test1.acx`. If you get an error about not being able to open
`standard.arch`, then `--include` is not pointing at the directory where the
library files live.

To play the adventure, type:

```shell
archetype --perform=test1.acx
```

You can also skip the binary while you are working, and compile and run in one
step:

```shell
archetype --source=test1.arch --include=games
```

Not much to do, is there? Find out just what you can do:

```
What should I do now? help
```

You will see a list of some of the verbs that the program understands. Synonyms
will not appear in this list. In other words, if "shoot" and "kill" are defined
simply as synonyms for "attack", only the verb "attack" shows up.

Notice that the system verbs — help, save, load, quit — and the direction verbs
— north, south, east, west, northeast, northwest, southeast, southwest, up, and
down — are all in this list. These verbs were defined in the `standard.arch`
file that you included when you typed `include "standard"` at the top of your
program.

Other things were defined as well, for example the `room` type. When you typed
`room first_room`, it created a room object named `first_room`. The properties
of this room were defined in the library: the ability to have exits in the
cardinal directions, the ability to contain any objects dropped in it,
descriptions that vary based on whether or not this is the player's first visit
to it, and so forth.

The `'INITIAL'` message and its significance were also defined in the library.
Before anything else starts in the game, this message is broadcast to every
object defined — objects you created as well as those the library created. By
putting this message in the `methods` section and following it with a colon and
a statement, you indicated that `first_room` had some specific action to take
when it received the `'INITIAL'` message.

In this case, the action is to send the message `'START HERE'` to the `player`
object. The player object is also defined in the library; it contains
attributes having to do with the player's location, how much they can carry,
and so on. The player object defines the `'START HERE'` message, and has a
method for it which causes it to move into the room which sent the message.

The `->` symbol, meaning "send this message to that object", is not defined in
the library. Neither is the word `include`, the word `methods` or `end`, or
even the general structure of your room definition. Those are defined by
Archetype itself.

However, it is important to remember that much of what you will come to think
of as "Archetype" is actually `standard.arch`, and especially `intrptr.arch`,
which are themselves written in Archetype! Once you know more about writing
adventure games, you might want to go in yourself and make your own
modifications.

HOWEVER IT IS AN EXTREMELY BAD PLAN TO MAKE CHANGES TO `intrptr.arch` UNLESS
YOU ARE VERY CERTAIN YOU KNOW WHAT YOU ARE DOING. In fact you should never
actually change `intrptr.arch`. You should make a copy with a different name
which you use yourself. If you break `intrptr.arch`, adventure games you make
with it will work erratically or not at all. Beware!

But enough with the alarmist injunctions. On to more and greater things.

## Describing a room

First of all, how about giving your room an interesting description. Right now
it's just "a nondescript room." The room's description is one of its
attributes, named `desc`. Attributes are listed before the word `methods` when
an object is described. You might change your object definition as follows:

```
room first_room

  desc : "room with the word START painted on the ceiling"

methods

  'INITIAL' : 'START HERE' -> player

end
```

The `desc` attribute is defined as `"nondescript room"` in the `room` type; if
you don't redefine it yourself, your room object will inherit that value just
by virtue of being based on the `room` type. The interesting attributes and
methods of the `room` type are shown below:

```
class room based on inventoried

  IsAroom   : TRUE
  visited   : FALSE
  desc      : "nondescript room"

  intro       : main.voice.Who & " can see"
  empty_intro : "There is nothing of interest here."

methods

  'ENTERED'  : ...
  'ROOMVIEW' : ...

  'BRIEF'    : write "In the ", desc, "."
  'LONGDESC' : write main.voice.WhoIs & " in the ", desc, "."

end
```

**Attributes**

| | |
|---|---|
| `IsAroom` | Always TRUE. It should not be changed. If every type defines an attribute called `IsA<type name>`, it makes it easy to test the type of an unknown object with a test such as `if obj.IsAroom then...` |
| `visited` | FALSE until the player has been here. The library maintains it. |
| `desc` | A short description of the room. It should not contain any detailed description; just the name of the room. It should also not contain any article at the front such as "a", "an", or "the". |
| `intro` | What is written at the beginning of the room's inventory list when the player types "look". If the room is dark, you might want to define it as "I can barely make out" or something. But you will usually want to leave it alone. |
| `empty_intro` | What is written when the room is empty. Again, you will usually want to leave this one alone. |

**Methods**

| | |
|---|---|
| `'FIRSTDESC'` | Invoked when the player first enters the room, and never again. A good place to put description that causes your story to unfold, or to put first-time traps or surprises. Remember that more than just written output can appear in a method. The type does not define it; you supply it if you want it. |
| `'LONGDESC'` | Invoked whenever the room is first entered or when the player types "look". |
| `'BRIEF'` | Invoked whenever the player re-enters a previously entered room. Usually just a brief reminder of what the room is. This is what helps your adventure feel like a book that is unfolding: `'FIRSTDESC'` is the reactions the character has initially and never again, `'LONGDESC'` is the result of a long slow look, and `'BRIEF'` is the character just passing through. |
| `'ENTERED'` | Sent by the library when the player arrives. It decides among the three above; you would only redefine it to change that policy. |

## Exits

The `methods` section also contains the exits out of the room. The name of the
message should be the name of the exit. It should return a single value — the
room into which it exits. For example, a room with the following methods:

```
'east' : kitchen
'west' : if door.open then bedroom
```

will have an exit to the east leading into the kitchen, and, if the `open`
attribute of the `door` object is TRUE, an exit to the west. Methods always
return a value to the sender: the last expression executed within them. The
`'east'` method will always return the `kitchen` object; the `'west'` method
returns the value of an `if` statement, which in turn returns its last executed
expression.

Suppose we fill out our example room a little more:

```
room first_room

  desc : "room with the word START painted on the ceiling"

methods

  'INITIAL' : 'START HERE' -> player

  'LONGDESC' :
    write "I'm standing in a small bare room with ",
          "the word \"START\" painted on the ceiling in neon ",
          "green paint.  The plaster is chipped a little and ",
          "the dirty white linoleum floor is something I ",
          "wouldn't want to have to touch with anything ",
          "but my shoes."

  'FIRSTDESC' :
    write "Welcome to my first adventure!  Hope you enjoy it.  ",
          "I was walking innocently along the sidewalk on my ",
          "way to the store one day when a flash of light just ",
          "to my left blinded me.  When my vision cleared I ",
          "found myself... somewhere else... without ",
          "explanation..."

# Exits

  'north' : hallway            # no other obvious way

end
```

There are a few new things here.

The pound sign (`#`), the comment, causes itself and everything to the end of
the line to be ignored by the compiler. You can use this to make notes to
yourself, or to someone else who might need to read it later.

Note the usage of the `write` statement. It takes any number of
comma-separated expressions and writes them out to the screen in such a way
that they wrap at the end of a line, like a word processor. This way you don't
have to worry about fitting them carefully to screen margins yourself.
Furthermore, if you have a very long description that goes beyond the screen,
Archetype will pause and wait for a keypress, so that none of the text is lost
off the top.

Also notice the `\"` used in the `'LONGDESC'` method. We want double quotes
within double quotes, but of course the presence of `"` means the end of a
string. Putting a backslash in front of a double quote means that the double
quote is part of the ongoing string.

For long stretches of prose there is a second way, which is often easier to
read because what you see is what you get. A line beginning with `>>` writes
everything after it, to the end of the line, and consecutive `>>` lines are
gathered into a single wrapped paragraph:

```
  'FIRSTDESC' : {
    >>Welcome to my first adventure!  Hope you enjoy it.  I was walking
    >>innocently along the sidewalk on my way to the store one day when a
    >>flash of light just to my left blinded me.
    }
```

### More about exits

An exit usually goes both ways. Take a look at the north exit in your first
room. It points to the hallway, which we have not yet defined. Define it as
follows:

```
room hallway

  desc : "long north-south hallway"

methods

  'LONGDESC' : >>I'm standing in a narrow hallway north of the START room.

  'south' : first_room

end
```

Before any other exits you might define for the hallway, you should first make
an exit going south back to the original room. Otherwise the north exit from
`first_room` will be a one-way exit. This is something to watch out for, since
no room has any exits other than the ones you define for it. Of course,
sometimes you might do this on purpose: a trapdoor or hole, for example.

## Objects

How about putting some objects in your adventure? Objects that you can pick up
and carry around? Try the following:

```
object necklace

  desc     : "diamond necklace"
  location : first_room

end
```

Be sure that all your room and object definitions are in the same `test1.arch`
file. Compile it again and run it. Notice that without any other information
about the diamond necklace, you can pick it up, look at it, drop it, and so
forth, referring to it as "the diamond necklace", "a diamond necklace", "the
necklace", "a necklace", or just "necklace". Of course, the necklace itself
isn't very interesting yet, since you didn't define anything special for it. It
merely inherited the attributes of the `object` type, enough structure to allow
it to behave like an object. The interesting inherited attributes are:

```
class object based on inventoried

  IsAobject : TRUE
  IsAnoun   : TRUE

  desc      : "nondescript object"
  full      : UNDEFINED
  syn       : UNDEFINED
  proper    : (desc leftfrom 1) within "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

  visible   : TRUE
  location  : UNDEFINED
  pronoun   : "it"

  size      : 1
  capacity  : UNDEFINED

end
```

| | |
|---|---|
| `IsAobject` | Serves the same function as `IsAroom` in the room type above. However it is even more important, since the interpreter depends on it. |
| `desc` | A very important attribute; it must almost always be defined. It is the full name of your object the way it appears on screen, and, if `full` is not defined, indicates how the player can talk about it as well. The interpreter assumes that you have defined `desc` in the form "adjective adjective ... noun", so that if `desc` were "great big red ball", the player could refer to it as "ball", "red ball", "big red ball", or "great big red ball", but not "great ball" or any other combination. |
| `full` | The full name of your object; the way the player would talk about it at the prompt. You need this attribute if you don't want to allow some of the names that the interpreter might generate from your `desc`. One example is "ball of wax", where the interpreter will accept "of wax" as a valid name. Set `full` to "ball of wax" and set `syn` to "ball\|wax". |
| `syn` | Synonyms for your object, separated by vertical bars. You can use this whether or not you define `full`. For an object whose `desc` is "scientific calculator", setting `syn` to "calc" lets the player say "calc" as well as the names generated from `desc`. |
| `proper` | Whether the object's name is a proper noun, guessed from whether `desc` begins with a capital letter. A proper noun gets no article: the game says "Ashley", not "the Ashley". |
| `visible` | Whether the object shows up in an inventory of the room. Usually TRUE; you might make it FALSE if the object is not really what the player would think of as an object, like a magic word. If your adventure required the player to type "say sesame", there would have to be an object named "sesame" to handle the action; it would have `visible` set to FALSE. |
| `location` | Another very important attribute. It is the location of the object and indicates the object's physical position. NOTE: just setting this attribute is not enough to actually change its position. It must be set and then the `'MOVE'` method must be invoked. |
| `pronoun` | What pronoun should be used for the object. Although usually "it", it can be any pronoun: "them", "him", "her". If the player has just mentioned this object, they can continue to refer to it by this pronoun, until they refer to another object with the same pronoun. |
| `size` | The size of the object. Size in what? Cubic inches? Square feet? Well, you decide. It is used to determine how much more the player can carry. If it is zero or UNDEFINED, the object has no significant weight. When the total of all the sizes the player is carrying exceeds `player.capacity`, the player gets "I can't carry any more." |
| `capacity` | How much can be put inside this object. When the total of the sizes of the objects whose `location` points here exceeds `capacity`, no more objects can be put inside. |

NOTICE that the most important attributes, the ones without which you would not
be able to see or mention the object, are `desc` and `location`. Without
`desc`, the interpreter cannot put its name on screen or know what names the
player can use. Without `location`, the object is not actually anywhere within
your game. All the others default to reasonable values.

### Methods

There are no methods which are mandatory in order to make your object
accessible. The necklace above required only two attributes to be part of the
game. But the `methods` section of an object contains three kinds of messages:

1. **System messages**, like `'DEF'`, `'INDEF'`, `'BEFORE'`, `'AFTER'`. These
   are in all uppercase letters. It is a bad idea to define your own messages
   in all capitals; it only increases the possibility of colliding with the
   system messages.
2. **Verb messages**, like `'get'`, `'north'`, `'kill...with'`. These are all
   lowercase. How to define your own verbs is covered below.
3. **General purpose messages.** Any other message you put in the `methods`
   section, just to group commonly invoked actions together. To conflict with
   neither system messages nor verbs, these are usually written with the first
   letters capitalized, like `'Wake Up'` or `'Begin'`.

## Parsing and verb messages

When the player enters a command, the interpreter puts the subject of the
sentence in `main.subj` and the direct object into `main.dobj`. It then creates
a single verb message that incorporates both the main verb and any
prepositional phrases, and sends that message to `main.subj`. (For
completeness, the verb goes into `main.verb` and the preposition into
`main.prep`.) The message is constructed as follows:

- If there is just one main verb phrase and a subject, the message is simply
  the verb phrase, words separated by one space. "get object" sends `'get'` to
  the object; "put down the object" sends `'put down'`.
- If there is a verb phrase, a subject, and a trailing preposition, the message
  is the verb phrase followed by an underscore and then the preposition. "pick
  the object up" sends `'pick_up'`; "take the coat off" sends `'take_off'`.
- If there is a verb phrase, a subject, a preposition, and a direct object, the
  message is the verb phrase followed by an ellipsis and the preposition. "take
  the uniform off of the dead guard" sends `'take...off of'` to the guard;
  "pick up the key with the wad of gum" sends `'pick up...with'` to the gum.
  Methods tied to messages with ellipses can count on `main.dobj` being
  defined.
- If there is a verb phrase but no subject or direct object, it is sent to the
  object that the player is "in". This is how hollow objects handle verbs like
  "leave" or "get out".

## Moving things around

One of the very first things you will probably want to do is move some object
from one place to another. This involves two things: changing the object's
`location`, and then sending it the `'MOVE'` message. If you forget the
`'MOVE'` message, the adventure will behave strangely. Sometimes the object
will act like it's there, and sometimes it won't. Even if an object changes its
own location, it still must send `'MOVE'` to itself.

The example here is a rock. When the rock is thrown at a glass vase, we want the
vase to shatter and disappear. The player types "throw the rock at the vase",
causing `'throw...at'` to be sent to the rock, with `main.dobj` set to
`glass_vase`:

```
object glass_vase
  location : first_room
  desc     : "glass vase"
end

object rock
  location : first_room
  desc     : "big ugly rock"
methods
  'throw...at' : {
    if main.dobj = glass_vase then {
      write "It shatters into miniscule pieces!  It's gone!"
      glass_vase.location := UNDEFINED   # Nowheresville
      'MOVE' -> glass_vase
      }
    else
      write "It bounces off of ", 'DEF' -> main.dobj, "."
    }  # throw...at
end
```

Note the `:=` there rather than `=`. An `=` would be a comparison, quietly
evaluated and thrown away, and the vase would not budge.

Note also the `'DEF'` message sent to `main.dobj`. It means the *definite* name
of the object. `'INDEF'` means the *indefinite* name. For the glass vase,
`'DEF'` returns "the glass vase" and `'INDEF'` returns "a glass vase". There is
also `'NEG INDEF'`, for *negative indefinite*, used in "I don't see ... here".
For most objects it is the same as `'INDEF'`, but for an object whose `'INDEF'`
is "some apples", `'NEG INDEF'` would be "any apples".

## Inheritance

How can the `glass_vase` object respond to messages like `'DEF'`, `'INDEF'`,
and `'MOVE'`, since they aren't defined in its `methods` section? Because they
were inherited from the `object` type. All of the attributes and methods of the
`object` type are part of `glass_vase` unless `glass_vase` redefines them.

## Doing things up front

There are some actions that have to be taken before the game even gets
underway, before the player has a chance to type the first command. Many of
these can be done just by initializing an attribute to the right value, like
setting the initial locations of all the objects. But sometimes the action
needs to be several statements. In that case, define an `'INITIAL'` method for
your object. Every object in the game receives `'INITIAL'` before the player
begins.

Every object then receives a second broadcast, `'ASSEMBLE'`, after all the
`'INITIAL'` methods have run. Use it for setup that depends on something
another object's `'INITIAL'` established.

## Doing things periodically

Sometimes there are actions that an object should take once per turn, or things
that should happen every so often. There is a `'BEFORE'` message sent out
before the player is asked for input, and an `'AFTER'` message sent out after
the player's actions have taken effect.

To receive these messages, an object has to ask up front to be on the list, by
sending `'REGISTER'` to the correct event handler: the object `before` for the
`'BEFORE'` message, and the object `after` for `'AFTER'`. Registering is
usually done in the `'INITIAL'` method. The object below puts out a random
scary message about once out of every ten turns:

```
object scary_sounds

methods

  'INITIAL' : 'REGISTER' -> before

  'BEFORE' : {
    if ?10 = 1 then
      case ?5 of {
        1 : write "I hear a far-off groan..."
        2 : write "A bat flaps by close overhead!"
        3 : write "I seem to hear a shuffling step behind me...!"
        4 : write "A sense of dread weighs on me..."
        5 : write "There is a clammy draft of air on the back ",
                  "of my neck..."
        } # case
    }  # BEFORE
end
```

In the example above, `scary_sounds` has no `desc` or `location` attribute,
since it isn't a tangible object. It only writes things to the screen.

For tangible objects with a `'BEFORE'` or `'AFTER'` method, however, it's
important to know whether the object is accessible to the player before doing
anything. Otherwise the object acts out of nowhere. The `'ACCESS'` method,
inherited from the `object` type, returns TRUE if the object is accessible and
FALSE if it is not. An object is accessible if the player has it, the player
and the object are in the same location, or the player is in it. The example
below is a monster that will roar in protest and snatch away the Wand of Power
if the player is carrying it:

```
object monster
  location : dungeon
  desc     : "monster"
methods
  'INITIAL' : 'REGISTER' -> after
  'AFTER' :
    if 'ACCESS' -> self and wand.location = player then {
      write "The monster lets out a roar of protest and ",
            "snatches ", 'DEF' -> wand, " from me."
      wand.location := self
      'MOVE' -> wand
      }
end
```

If the `'AFTER'` method didn't check `'ACCESS' -> self`, the monster would roar
and take the wand the moment the player acquired it, even if the player were
nowhere near the monster.

## Did I happen to mention...? (the 'MENTION' message)

The pronouns in Archetype usually refer to the last time an object with that
pronoun was referred to in a player command. Sometimes it may be appropriate,
however, to cause a different object to become the object of interest. Take,
for example, the following exchange:

```
What should I do now? pick up the wallet
I picked up the wallet.
What should I do now? open it
I opened the wallet.  A driver's license fell out!
What should I do now? get it
I already have the wallet.
```

At that point, the player probably expected "it" to refer to the most
interesting object just mentioned. This can be done by having the driver's
license send `'MENTION'` to the `main` object. Additionally, an object can be
directed to mention itself by sending it `'MENTION SELF'`. The code below
implements the situation above, with the difference that when the player
finally types "get it", they will pick up the license, not the wallet:

```
object wallet
  location : dresser
  desc     : "wallet"
methods
  'open' : {
    writes "I opened the wallet.  "
    if license.location ~= self then
      write "Nothing happened."
    else {
      write "A driver's license fell out!"
      license.location := player.location
      'MOVE' -> license
      'MENTION SELF' -> license
      }
    }
end
```

The conceit behind `'MENTION'` is that the whole transcript, the player's half
and the game's half together, is a shared conversation. Whatever was mentioned
most recently is the most salient thing, whoever mentioned it.

## Customizing the game environment

The interpreter is comprised of several objects, such as `main`, `player`, and
the various basic verbs. You can change certain default attributes of these
objects to give your adventure a different look and feel.

**`main.voice`** chooses the narrative person. It points to `first_person` by
default, which is why the game says "I'm in the bleak, bare cell" and prompts
"What should I do now?". Set it to `second_person` in your starting room's
`'INITIAL'` method and the whole library changes register: "You're in the
bleak, bare cell", prompting "What do you want to do now?". The library's own
messages are written in terms of `main.voice`, so this one assignment carries
through everything the standard objects say.

```
  'INITIAL' : {
    main.voice := second_person
    'START HERE' -> player
    }
```

Other attributes worth knowing:

| | |
|---|---|
| `main.prompt` | The string printed just before asking the player for input. It is built from `main.voice.prompt` and some ANSI formatting; assign it directly if you want something else entirely. |
| `main.wait_message` | Printed when the player simply types RETURN at the prompt. |
| `compass.intro` | Printed before the list of visible exits. |
| `compass.empty_intro` | Printed if there are no visible exits. |
| `player.intro` | Printed before listing the player's inventory. |
| `player.empty_intro` | Printed when the player is carrying nothing. |
| `player.capacity` | Generally the number of objects the player can carry before getting "I can't carry that much." It is actually the upper limit of the sum of the `size` attributes of the player's objects, so if any object has a size greater than one, the capacity is reached more quickly. Defaults to 8. |

Some other parts of the environment, like the strings printed when a room is
entered, cannot be changed by direct assignment, since they are part of a type
definition. But you can define your own type based on the library's type and
modify anything you want about it. Suppose that when the player enters a room
you want the game to say "You can see" before the list of visible objects, or
"Nothing here." if there aren't any:

```
type my_room based on room
  intro       : "You can see"
  empty_intro : "Nothing here."
end
```

After this, make sure that all your rooms are of type `my_room` and not of type
`room`.

Remember that the entire main interpreter, `intrptr.arch`, is written in
Archetype. Once you read [the Archetype manual](manual.md) you may be able to
understand the code in there better, and modify it to suit your taste.

## Disabling verbs

As in the previous examples, the handling of a verb is usually left up to the
main subject of the sentence. If there is some universal sense to a verb (like
`'get'`), then it is usually defined in a type on which objects are based, so
that all objects inherited from that type appear to handle it the same way.

Once in a while, however, it may make more sense to disable the verb at the
source, before the subject of the sentence gets a chance at it. You can do this
by setting the `disabled` attribute of the verb to a string which should be
printed instead of passing the message to the main subject.

One verb in particular, `lookV` (the verb `'look'`), will affect the
interpreter when disabled. No room descriptions will be printed upon entering a
room, and inventory will not respond either. Instead, `lookV.disabled` is
written. This is a convenient way to make the player blind if the lights should
go out.

## Defining your own verbs

Normally, the only verbs and prepositions that Archetype will parse and process
are those defined in `lexicon.arch`. If you want the parser to recognize a verb
or preposition of your own, you must define a `Verb` or `lex` object. The
interesting attributes of a `lex` object are shown below, using the preposition
"in" as an example:

```
type lex based on null

  full : 'in'                   # the full or "true" name
  syn  : 'inside|into'          # bar-separated synonyms, all translated
                                #  to .full if encountered
  on_menu : TRUE                # whether this shows up when the player
                                #  types "help"
end
```

The `full` attribute is what anything in `syn` becomes, before it is combined
with other verbs. If an object "box" defines a method for the verb message
`'look in'`, it will receive that message if the player types "look in the
box", "look inside the box", or "look into the box". NOTE that if "box" defines
a method for `'look inside'`, it will never be invoked: "inside" translates to
"in" before it is combined with "look".

These objects are usually never referred to by name within the adventure, so
they are left nameless by giving them a name of `null`:

```
lex null full : 'under'     end
```

The `Verb` type is based on `lex`, meaning that it has all of the above
attributes, plus:

```
type Verb based on lex

  disabled  : UNDEFINED         # see "Disabling verbs" above
  interpret : UNDEFINED         # who should receive the verb message
                                #  (if not main.subj)
  normal    : FALSE             # whether this verb has a normal,
                                #  "default" action
end
```

All three of these attributes have an associated method: `'DISABLED'`,
`'INTERPRET'`, and `'NORMAL'`. If an attribute is neither FALSE nor UNDEFINED,
the method associated with it is invoked.

The typical way to disable a verb is simply to assign a value to its `disabled`
attribute. However, if you are defining your own verb, you can supply a
`'DISABLED'` method which does something other than just write the string.
Remember that the verb's `disabled` attribute must still be set for its
`'DISABLED'` method to be invoked.

The `'INTERPRET'` method, invoked if `interpret` is defined, normally just
makes sure that `'ACCESS' -> interpret` is true, then sends its `full`
attribute to whatever object `interpret` points to. Be sure that you assign an
*object* to this attribute, not a string as with `disabled`.

The `'NORMAL'` method is invoked if `normal` is defined, and if the recipient
of the verb message doesn't have a method defined for the verb. By default
`'NORMAL'` just writes the string in `normal`. Here's the verb `'kill'`, as it
is defined for The Gorreven Papers:

```
Verb null
  full : 'kill'
  syn  : 'attack|fight'
  normal : "That's not the kind of thing I can kill."
end
```

Whenever the player tries to kill something, or attack it, or fight it, and the
object named has no method defined for `'kill'`, the game prints "That's not
the kind of thing I can kill."

If there is a more complicated default action, `normal` can simply be set to
TRUE and the `'NORMAL'` method defined to be as complicated as necessary. Here
is another example from The Gorreven Papers, this time for `'search'`:

```
Verb null
  full : 'search'
  syn : 'examine'
  normal   : TRUE
methods
  'NORMAL' :
    if main.subj.IsAhollow_object then {
      >>I get closer for a more thorough look.
      player.location := main.subj; 'MOVE' -> player
      }
    else if main.subj.IsAguard_type and main.subj.alive then
      write "He's not going to permit that kind of scrutiny; ",
            "not while he's alive."
    else if ('look' -> main.subj) = ABSENT then
      write "I find nothing unusual even upon a close search."
end
```

The `'NORMAL'` method will be invoked if the player tries to search an object
that has no `'search'` method defined. Note at the bottom that it finally sends
`'look'` to `main.subj`, so that a search is at least as good as a look.
However, if that sending returns ABSENT, showing that there is no `'look'`
method defined for the object, then it writes out a message of its own.

This is more important than it probably seems. If sending that verb to
`main.subj` is ABSENT, it means that nothing was done at all, and the player
will be greeted with a blank line. Of course, if a `'look'` method was defined
anywhere in `main.subj`'s ancestry, so that `main.subj` could inherit it, then
that is the method that will have been invoked.

Another way of defining a default action for a verb is to create a type based
on the `object` type and then define everything in your game as being of that
type, so they all inherit the default handlings. The methods just described,
however, allow you to customize your own verbs without having the cooperation
of a carefully planned inheritance tree.

The order of handling is:

1. The interpreter determines both the verb and the verb message. (If the
   player typed "open the box with the key", then `'open'` is the verb and
   `'open...with'` is the verb message.)
2. If the verb has its `interpret` attribute defined, `'INTERPRET'` is sent to
   it, and nothing more is done; the verb is expected to handle the rest.
   Remember that the inherited `'INTERPRET'` method simply sends the verb
   message to the object pointed to by `interpret`. Thus, if you have a verb
   better handled by the direct object than the subject, `interpret` can be
   defined as `main.dobj`.
3. If the verb has its `disabled` attribute defined, `'DISABLED'` is sent to
   it. Nothing more is done.
4. Steps 2 and 3 are done for the verb *message*. In other words, it is
   possible to define a `Verb` object whose `full` attribute is something like
   `'shoot...with'`.
5. The verb message is sent to the subject of the sentence. If that object has
   a method defined for that message, nothing more is done.
6. If the verb message is the full name of a `Verb` object, and it has its
   `normal` attribute defined, `'NORMAL'` is sent to that `Verb` object.
7. If the verb alone has a `normal` attribute defined, `'NORMAL'` is sent to
   that `Verb` object.

## Learning more

The sample game `bare.arch` is a bare-bones adventure that manages to show off
many of the features described here, plus a few more. It is heavily commented
to help walk you through the flow of the code. If the comments get in the way,
look at `barer.arch`, which is the same adventure without them.

Once you become familiar with [the Archetype Language Reference
Manual](manual.md), which describes all the aspects of the language proper, you
will be able to read the `.arch` files that make up the interpreter, and
perhaps write your own interpreter with a different flavor. The program
`animal.arch` demonstrates how an entirely different kind of game can be
written with Archetype — in this case the old Animal game, where the computer
tries to guess what animal you are thinking of, and has you teach it when it
gets one wrong. Try it, and look at the code.

Enjoy!
