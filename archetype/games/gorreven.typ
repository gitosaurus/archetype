# GORREVEN.TYP
#
# Type definitions for "The Gorreven Papers".

include "utility"

type sit_furniture based on furniture

  proximity     : "on"

methods

  'look'        : >>It looks like I could sit on it.

  'sit'         : 'enter'
  'sit on'      : 'sit'
  'sit down on' : 'sit'

end


type sitat_furniture based on sit_furniture

  proximity : "at"

methods

  'look'        : >>Looks like I could sit at it.

  'sit at'      : 'sit'

end


type wearable based on object

  wearing : FALSE

methods

  'INVENTORY NAME' :
    if visible then {
      if wearing = TRUE then
        ('INDEF' -> self) & " (which I'm wearing)"
      else if wearing.IsAobject then
        ('INDEF' -> self) & " (which " & ('INDEF' -> wearing) & " is wearing)"
      else
        'INDEF' -> self
      }

  'put_on' : 'wear'
  'wear' :
    if wearing = TRUE then
      write "I'm already wearing ", 'DEF' -> self, "."
    else if location ~= player then
      write "I don't have ", 'DEF' -> self, "."
    else {
      write "I put on ", 'DEF' -> self, "."
      wearing := TRUE
      }

  'take_off' : 'remove'
  'remove' :
    if wearing = TRUE then {
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



type guard_type based on object

  IsAguard_type : TRUE

  alive : TRUE
  desc  : 'Desc' -> self

  suspicious : 0
  see_naked  : 0
  panic      : 0

  pronoun : "him"

methods

  'Desc' :
    if alive then
      "guard"
    else
      "dead guard"

  'INITIAL' : 'REGISTER' -> after

  'AFTER' :
    if alive and 'ACCESS' -> self then 'Encounter'

  'get'  :
    if alive then
      write "Oh sure."
    else
      write "I don't want to burden myself with that."

  'kill' :
    if not alive then
      write "He's already plenty dead."
    else if gun.location = player and gun.bullets > 0 then
      { writes "I use my gun.  "; 'Kill With Gun' }
    else if cot_fragment.location = player then
      { writes "I use the piece of metal that worked so well before..."
        'Kill With Knife' }
    else {
      writes "I attack with my bare hands grasping for his throat. "
      writes "But this turns into a monumental failure as he triumphantly "
      stop "drills me through the skull with his sidearm."
      }

  'kill...with' :
    if not alive then
      write "He's already dead."
    else
      case main.dobj of {
        cot_fragment : 'Kill With Knife'
        gun          : if gun.bullets > 0 then
                         'Kill With Gun'
                       else
                         write "The gun's out of bullets!"
        cattle_prod  : 'Kill With Prod'
        bear_trap    : stop "Not knowing quite how I plan to kill him with ",
                            "a bear trap, of all things, I sort of open its ",
                            "jaws and lunge at him.  Unfortunately he is ",
                            "already prepared with his gun and a gun beats ",
                            "a bear trap anytime."
        default      : >>That won't make much of a weapon!
        }

  'Kill With Knife' : {
    writes "Sadly, between a sharp piece of metal and a .38 caliber pistol, "
    writes "the firearm comes out on top.  He has perforated me soundly "
    stop "long before I am within striking distance."
    }

  'Kill With Gun' : {
    gun.bullets -:= 1
    writes "I fire, point blank; he was not ready and I was.  He is thrown "
    writes "against the far wall by the impact and slumps to the ground, "
    writes "painting a bloody stripe down the wall behind him, his face "
    write "frozen in a death mask of bewilderment."
    alive := FALSE
    }

  'Kill With Prod' :
    stop  "I shove the cattle prod into him, but the charge in the prod ",
          "isn't sufficient to kill anyone... he yelps in pain, then, in ",
          "a paroxysm of rage, pulls his gun and shoots me."

  'pull' : 'push'
  'push' : {
    writes "I ", main.verb, " ", 'DEF' -> self, ".  "
    if not alive then
      write "Nothing happens."
    else {
      writes "He seems surprised at the sudden physical contact.  Annoyed, ",
             "he turns and looks me right in the eye.  "
      'Found Out'
      }
    }

  'Encounter' :
    if power.off then {
      case panic +:= 1 of {
        1 : write "The guard is on edge in the wake of the power outage ",
              "I caused.  He is nervous and trigger happy and staring ",
              "wide-eyed into the dark.  Fortunately he can't see my ",
              "face yet and he simply acknowledges me with a nod."
        3 : { writes "The guard's eyes seem to have adjusted a little to the ",
              "dark.  He sees my face, and his eyes grow wider still.  "
        'Found Out'
          }
        }
      }
    else { if uniform.wearing = TRUE then
      case suspicious +:= 1 of {
        1 : {
          write "Looks like ", 'DEF' -> self, " isn't paying very close ",
                "attention to my face."; 'MENTION SELF' }
        2 : {
          write "The disguise seems to be holding; ", 'DEF' -> self,
                " doesn't seem suspicious yet."; 'MENTION SELF' }
        4 : {
          write "Now ", 'DEF' -> self, " seems to be a little disturbed, ",
                "trying to get a good look at my face."  'MENTION SELF' }
        5 : {
>>The guard suddenly draws his weapon.  "You're not Krugnik!  Who are you?
>>And what have you done with him?"  Startled, I make the fatal mistake of
writes "glancing up and making eye contact.  "
          'Found Out'
          }
        }     # case
    else {                      # not wearing prison uniform
      writes "A look of shock comes over ", 'DEF' -> self, "'s face "
      writes "as he sees me standing there in "
      if prison_coveralls.wearing then {
        writes "my gloriously conspicuous prison coveralls, the bright orange "
        writes "triangle glowing like a beacon.  Making a wonderful target.  "
        'Found Out'
        }
      else {
        writes "my stunning nakedness.  "
        case see_naked +:= 1 of {
          1 : { write "He seems unsure how to react."; 'MENTION SELF' }
          default : 'Found Out'
          }  # case
        }  # else           wearing nothing
      }  # else             not wearing prison uniform
      }  # kluge - should be able to do that 'else if'

  'Found Out' : {
    writes "\"The SPY!\" he shrieks, and before I can react further, "
    stop "he has shot me conclusively through the heart."
    }

end


# scenery:  these are objects which are really part of the surroundings,
# but will yield more information on a closer look.

type scenery based on object

  first_seen : TRUE

methods

  'get' : >>I can't do that.
  'look' : {
    'Desc'
    first_seen := FALSE
    }

end


type openable based on object

  open  : FALSE

methods

  'look' : {
    writes "It is "
    if open then writes "open " else writes "closed "
    write "at the moment."
    }

  'open' :
    if open then
      write "It's already open."
    else {
      open := TRUE
      write "I opened ", 'DEF' -> self, "."
      }

  'close' :
    if open then {
      open := FALSE
      write "I closed ", 'DEF' -> self, "."
      }
    else
      write "It's already closed."

end


type elev_button based on object

  location : elevator
  visible  : FALSE

  desc     : "button " & ('Name' -> self)
  syn      : level

  level    : 0

methods

  'look' : {
    writes "It's just a small square button with the number \"", level,
           "\" painted on it.  "
    if elevator.level = level then writes "It is currently lit."
    write
    }

  'get'  : write "I can't remove ", desc, " from the wall."

  'Name' :
    case level of {
      1 : "one"
      2 : "two"
      3 : "three"
      4 : "four"
      }

  'push' :
    if elevator.level = level then
      write "That's the button that's lit.  Nothing happens."
    else {
      writes "There is the sound of a mighty motor and "
      if level < elevator.level then
        writes "the floor briefly drops out from under me.  "
      else
        writes "my knees buckle with the force of the lift.  "
      elevator.level := level
      if elevator.never_rode then {
        elevator.never_rode := FALSE
        writes "To my shock, there is no inner door to this elevator.  The "
        writes "concrete bricks of the elevator shaft go hurtling by in a "
        writes "blur.  I don't like to think about what might have happened "
        writes "if I had chosen to lean against that wall.  "
        }
      if level = 4 then
        writes "The ride seems to be taking quite some time.  "
      writes "Finally, with a bone-jarring, sudden stop, the ride comes "
      write "to an end."
      }

end


type elev_call_button based on elev_button

  visible : TRUE
  desc    : "elevator call button"
  waiting : FALSE

methods

  'look' : {
    writes "The button is "
    if not waiting then writes "not "
    write "lit."
    }

  'push' :
    if elevator.level = level then {
      writes "The button lights briefly and there is the sound of a bell; "
      write  "this is because the elevator is already here."
      }
    else {
      write "I push the button and it becomes lit."
      'Calling Elevator' -> elevator
      waiting := TRUE
      }

end


# Thin-handled implement.  Basically the mop and the broom.  These objects
# can get a dustpan stuck on the end of them.

type thin_handled based on object

  is_shovel : dustpan.location = self

methods

  'look' : if is_shovel then
     write "There's ", 'INDEF' -> dustpan,
           " stuck on the end."
     else
       message --> object

  'TRANSPARENT' : TRUE
  'INVENTORY NAME' :
    if dustpan.location = self then
      ('INDEF' -> self) & " with a dustpan stuck on the end"
    else
      'INDEF' -> self

end


type elevator_hall based on room

  desc : "hallway near the elevator"
  level : 0
  IsAelevator_hall : TRUE

methods

  'south' : if elevator.level = level then elevator

  'Elevator Desc' :
    if elevator.level = level then
      writes "The elevator is waiting directly to the south.  "
    else {
      writes "There is a huge square gaping hole directly to the south.  "
      writes "It's not much of an exit; the rear of the elevator shaft is "
      writes "visible a few feet beyond.  "
      }

end



type fence based on object

  desc      : "chain-link fence"
  syn       : "chainlink fence"
  otherside : UNDEFINED
  never_seen : TRUE

methods

  'get'  : >>Yeah right.
  'look' : {
    writes "It's about twelve feet high, made of a chain-link construction ",
    "that is somewhat thicker than what I'm used to.  It seems to be buried ",
    "securely underground; probably not easy to tear up.  Oh, and there's a ",
    "sign on it saying \"HIGH VOLTAGE - KEEP OFF\".  Important detail.  "
    if never_seen then { never_seen := FALSE; hinter.hint := power_off }
    if power.off then
      writes "Hopefully, though, turning off the main power grid took care ",
      "of that.  Just one way to find out."
    write
    }

  'climb' :
    if power.off then {
      writes "I climb up the fence.  Nothing happens; the power is still off.  "
      if searchlight.broken then {
        player.location := otherside; 'MOVE' -> player
        }
      else
        stop "But halfway up the fence the burning light of a searchlight ",
             "from the central guard tower stabs down upon me.  ",
             "It must have been independently powered!  Like a ",
             "cockroach caught in the middle of a bathroom floor I leap away, ",
             "lunging toward the darkness.  But the searchlight follows me, ",
             "and I hear the popping sounds of gunfire.  An instant later my ",
             "body is torn by bullets; like an actor on a stage, I perform ",
             "my death scene beneath the spotlight."
      }
    else
      stop "I grab hold of the fence.  A horrible vibrating burning sensation ",
      "leaps through me.  The fence is electrified!  I try to let go but my ",
      "muscles are paralyzed!  I... I... AIIIIIIIEEEEEEEE..."

end
