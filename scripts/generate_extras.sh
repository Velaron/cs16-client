#!/bin/bash

# Specify the path to the file with the maps list
EXTRAS="build/extras.pk3"

python -m zipfile -c "$EXTRAS" 3rdparty/cs16client-extras/*

echo "Extras.pk3 successfully created."
