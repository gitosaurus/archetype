#!/bin/bash

Build()
{
    echo "Building Archetype:"
    echo "Create build directory"
    mkdir -p build && cd build
    echo "Build make files"
    cmake ../drivers/archetype .
    echo "Build the project"
    make
    echo
    echo "To use Archetype run build/archetype"
    echo
}

Clean()
{
    DIRECTORY=./build
    if [ ! -d "$DIRECTORY" ]; then
      echo "build directory does not exist."
    else
        read -r -p "Delete build directory? [y/N] " response
        case "$response" in
            [yY][eE][sS]|[yY]) 
                echo "Deleting build directory"
                rm -rf $DIRECTORY
                ;;
            *)
                echo "Deleting cancelled"
                ;;
        esac
    fi
}

Help()
{
   # Display Help
   echo "Build archetype for Linux."
   echo
   echo "Syntax: "$0" [-b|c|h]"
   echo "options:"
   echo "b   Build the project."
   echo "c   Clean the project (deleted build directory)."
   echo "h   Print this Help."
   echo
}

while getopts ":bch" option; do
   case $option in
      h) # display Help
         Help
         exit;;
      b) # build
         Build
         exit;;
      c) # delete build directory
         Clean
         exit;;
      *)
         Help
         exit;;
   esac
done

Help
