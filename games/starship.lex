# STARSHIP.LEX
#
# Verbs defined especially for STARSHIP.ACH .

lex null full  : 'sit'    syn : 'climb|lie|lay' end
lex null full  : 'onto'   syn : 'on to|on top of' end
lex null full  : 'top'    end
Verb readV
  full : 'read'
  disabled : 'Find Out' -> self

methods

  'Find Out' :
    if lookV.disabled then
      lookV.disabled
    else if sunglasses.wearing then
      "I can't see well enough to read with these sunglasses on."
    else
      UNDEFINED

end
Verb null full  : 'turn'   normal : "That doesn't make sense." end
Verb null
  full  : 'patch'
  syn : 'repair|fix'
  normal : "That doesn't need fixing."
end
Verb null full  : 'drink' end
Verb null full  : 'fill'   end
Verb null full  : 'pull'   normal : "Nothing happens." end
Verb null full  : 'push'   syn : 'press'  normal : "Nothing happens." end
Verb null full  : 'start'  end
Verb null full  : 'kick'
  normal : TRUE
methods
  'NORMAL' : write "I give ", 'DEF' -> main.subj, " a swift kick.  Nothing ",
                   "happens."
end
Verb null full  : 'plug'   end
Verb null full  : 'unplug' end
lex null full  : 'from'   end
lex null full  : 'through'    syn : 'thru'  end
lex null full  : 'against' end

# Special verb combinations

Verb null
  full      : 'look up'             syn : 'look up...in|look...up in'
  interpret : tech_index
end

Verb null
  full      : 'shoot'               syn : 'shoot...with'
  interpret : hand_blaster
end

Verb null
  full      : 'clean'               syn : 'clean...with'
  interpret : windex
methods
  'INTERPRET' :
    if not main.subj then
      write "What do you mean, \"clean\"?"
    else if 'NOUNS PRESENT' -> main then {
      if main.subj ~= helmet then
        write "I don't think ", 'DEF' -> main.subj, " needs any cleaning."
      else if not main.dobj then
        write "I try to clean ", 'DEF' -> main.subj,
              " with my bare hands, but it doesn't work... I only smudge ",
              "the dirt further."
      else
        case main.dobj of {
          windex : {
            write "I spray ", 'DEF' -> main.subj, " with ", 'DEF' -> windex,
                  ".  Now there's just a muddy mixture of dirt and windex on ",
                  "it... I could use a towel."
            helmet.dirty := Windexed
            }
          towel  :
            if not helmet.dirty then
              write "I run the towel over the already squeaky clean helmet."
            else if helmet.dirty = Windexed then {
              writes "I clean off the Windex.  Wow!  What a shine on that "
              write "faceplate!"
              helmet.dirty := FALSE
              }
            else
              write "The towel serves only to smudge the dirt further.  ",
                    "I should probably clean it with Windex first."
          default :
            write "That's not something to use to clean a helmet."
          }
    }  # nouns are present

end


Verb null  full : 'wear'  syn  : 'put on' end
Verb null full : 'remove'  end

