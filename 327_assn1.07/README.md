Assignment 1.07 - CSV Parsing

This project parses specific csv files within the pokedex database. It checks two file locations, the shared on pyrite and a home location. When running please type the file of your choice to run, for example ./Assignment1_07 pokemon to run pokemon.csv. The files available are listed in the assignment specs, and when ran it stores the data, and prints it out.

New functions:
- `verify_file:` Checks to see if the file given is a valid file name.
- `print_file:` Prints out the file data/information.
- `read_file:` Reads the corresponding file and puts the data into the correct places. Calls print_file at the end to print out the data.
- `parse:` Finds the file/path location. Checks the shared location first, then the home, and if in neither it will exit. If it does find the path then it'll run read_file.
- `main:` Not a new funciton, but I added checking if a file name was listed when running, then verifying the file name and finally parsing it. Immediately after it exits, so the actual game won't run.

New data structures:
- `pokemon:` Structure to contain pokemon.csv data.
- `moves:` Structure to contain moves.csv data.
- `pokemon_moves:` Structure to contain pokemon_moves.csv data.
- `pokemon_species:` Structure to contain pokemon_species.csv data.
- `experience:` Structure to contain experience.csv data.
- `type_names:` Structure to contain type_names.csv data.
- `pokemon_stats:` Structure to contain pokemon_stats.csv data.
- `stats:` Structure to contain stats.csv data.
- `pokemon_types:` Structure to contain pokemon_types.csv data.
