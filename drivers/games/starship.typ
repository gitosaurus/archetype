# STARSHIP.TYP
#
# Special type definitions used in STARSHIP.ACH .


include "utility"

type wearable based on object

  wearing : FALSE

methods

  'INVENTORY NAME' :
    if visible then
      if wearing then
        ('INDEF' -> self) & " (which I'm wearing)"
      else
        'INDEF' -> self

  'put_on' : 'wear'
  'wear' :
    if location ~= player then
      write "I don't have ", pronoun, "."
    else if wearing then
      write "I'm already wearing ", 'DEF' -> self, "."
    else {
      write "I put on ", 'DEF' -> self, "."
      wearing := TRUE
      }

  'take_off' : 'remove'
  'remove' :
    if wearing then {
      write "I take off ", 'DEF' -> self, "."
      wearing := FALSE
      }
    else
      write "I'm not wearing ", 'DEF' -> self, "."

# And remember to change how drop works.

  'drop' : {
    if wearing then 'remove'
    'drop' --> object
    }

end


# Plural nouns or mass nouns.
# described as indefinite nouns.

type plural_noun based on object

  pronoun : "them"

methods

  'INDEF' :
    "some " & desc

end



# large_structure
#
# This is a structure that you can enter, something much like a room but
# not accessible by the cardinal directions.  You cannot 'get' this kind
# of object; the reason why not is given in the why_not attribute.

keyword PartOfRoom, PartOfFloor, FastenedDown, TooHeavy, Impossible

type large_structure based on hollow_object

  why_not   : PartOfRoom

methods

  'look in' :
    if 'TRANSPARENT' -> self then
      message --> hollow_object
    else if open = FALSE then
      write "I can't look inside that; it isn't open."
    else
      write "I can't look inside that."

  'get' : {
    writes "I can't pick that up; "
    case why_not of {
      PartOfRoom   : >>it's part of the room.
      PartOfFloor  : >>it's part of the floor.
      FastenedDown : >>it's securely fastened down.
      TooHeavy     : >>it is much too heavy.
      Impossible   : >>it would be rather impossible.
      }
    }

end



type big_furniture based on furniture

methods

  'look'  : >>It looks big enough to go to.
  'look in' : message --> large_structure
  'get' :
    >>That's too bulky to pick up.

end


type sit_furniture based on big_furniture

  proximity       : "on"

methods

  'look'          : >>Looks big enough to sit on.
  'sit'           : 'enter'
  'sit on'        : 'enter'
  'sit onto'      : 'enter'
  'get onto'      : 'enter'

  'get up'        : 'leave'

end



# visible_part
#
# These are things that are visible parts of other objects, such as latches
# or doorknobs.  You can manipulate them otherwise but you cannot 'get' them.
# What it's part of is listed in part_of.  "wall" is an enumerated constant
# meaning simply part of the surrounding walls.

keyword  wall

type visible_part based on object

  part_of   : wall

methods

  'get' : {
    writes "I can't pick that up; it's part of "
    if part_of = wall then
      write "the wall."
    else if 'DEF' -> part_of = UNDEFINED then
      write part_of, "."                        # it will be text
    else
      write 'DEF' -> part_of, "."
    }

end


type invisible_part based on visible_part

  visible  : FALSE

end



# Batteries-not-included objects.

type needs_batteries based on openable

  IsAneeds_batteries : TRUE
  contains : UNDEFINED
  open     : FALSE

methods

  'TRANSPARENT' : open

  'Insert Me' :
    if not sender.IsApowersource then
      write "I don't think ", 'DEF' -> sender, " is a power source."
    else if not open then
      write "I can't because ", 'DEF' -> self, " is not open."
    else {
      sender.location := self; 'MOVE' -> sender
      write "I put ", 'DEF' -> sender, " into ", 'DEF' -> self, "."
      }

  'Change Open' :
    main.verb --> openable

  'look' :
    if open then {
      writes "It is presently open."
      if contains then {
        writes "  There is ", 'INDEF' -> contains, " lying inside.  "
        writes "However it is not quite sitting flat; it probably would if "
        writes "I closed ", 'DEF' -> self, "."
        }
      write
      }
    else
      write "It is presently closed."

end



type powersource based on object

  IsApowersource : TRUE

  powered   : TRUE

methods

  'MOVE' :
    if location.contains then {
      write "There's ", 'INDEF' -> location, " in the way."
      location := last_location
      }
    else {
      if last_location.IsAneeds_batteries then
        last_location.contains := UNDEFINED
      'MOVE' --> object
      if location.IsAneeds_batteries then location.contains := self
      }

  'put...in' :
    if main.dobj.IsAneeds_batteries then
      'Insert Me' -> main.dobj
    else
      message --> object

end


# gravhalf
#
# An object that is fitted with the other half of a gravlifting apparatus.

type gravhalf based on object

  IsAgravhalf : TRUE
  prev_weight : 0

  outlet_filled  : UNDEFINED

methods

  'look' : {
    writes "There's a slim green ring around the bottom with a "
    writes "five-hole outlet in it"
    if outlet_filled then {
      write "; ", 'INDEF' -> outlet_filled, " is plugged into the outlet."
      'MENTION SELF' -> outlet_filled
      }
    else
      write "."
    }

  'Plug Into' : {
    outlet_filled := sender
    sender.location := self; 'MOVE' -> sender
    if sender.location = self then {
      write "I plugged ", 'DEF' -> sender, " into ", 'DEF' -> self, "."
      if gravlifter.powered then {
        prev_weight := size
        size := 0
        write "Lights around the tray on the bottom light up."
        }
      else
        write "Nothing happens."
      }
    }

  'Unplug From' : {
    outlet_filled := UNDEFINED
    sender.location := location; 'MOVE' -> sender
    if sender.location = location then {
      write "I unplugged ", 'DEF' -> sender, " from ", 'DEF' -> self, "."
      if sender.powered then {
        size := prev_weight
        if location = player then {
          writes "Oof!  Suddenly ", 'DEF' -> self, " regains all of its "
          write "previous weight and crashes to the floor!"
          location := player.location; 'MOVE'
          }
        }
      else
        write "Nothing happens."
      }
    }

end



type readable based on object

  viewed : FALSE

methods

  'look' : >>There's writing on it.
  'read' : >>I can't quite make out anything.

end



