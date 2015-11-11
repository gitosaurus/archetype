# GORREVEN.LEX
#
# Extra lexicon for "The Gorreven Papers".

Verb null full : 'sit' end

Verb null
  full : 'kill'
  syn  : 'attack|fight'
  normal : "That's not the kind of thing I can kill."
end

Verb null
  full   : 'face'
  normal : "I set my shoulders and face " & ('DEF' -> main.subj) & "."
end

Verb null
  full   : 'break'
  syn    : 'smash'
  normal : "It won't break."
end

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
      write "He's not going to permit that kind of scrutiny; not while he's alive."
    else if ('look' -> main.subj) = ABSENT then
      write "I find nothing unusual even upon a close search."
end


lex null full : 'from' end


Verb null full : 'wear'    syn : 'put on' end
Verb null full : 'remove'  syn : 'take off' end


Verb null full : 'open'  end
Verb null full : 'close'       syn : 'shut' end

Verb null
  full : 'push'
  syn  : 'press'
  normal : "I push it but nothing happens."
end

Verb null
  full : 'pull'
  normal : "I pull on it but nothing happens."
end

lex PutInto
  full : 'put...in'
  normal : TRUE
  on_menu : FALSE
methods
  'NORMAL' : if main.dobj = moat then 'Drop Subj In' -> moat else ABSENT
end

Verb null
  full : 'climb'
  normal : "Climb " & ('DEF' -> main.subj) &"?  That doesn't make any sense."
end

Verb Turn
  full : 'turn'
  normal : "Turn " & ('DEF' -> main.subj) & "? That doesn't make sense."
end
