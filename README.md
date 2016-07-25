## Usage

* Compile: make
* Run: ./kart_race #number_of_karts #laps
** ./kart_race 5 5

## Design
* Simple single Observer pattern. Each time a kart cross the start line, notify by whatever system the passing_time and the kart number (minimal information needed)
* Each Kart implement a DrivingStrategy with a single public method : *void drive() const*
** we could imagine diferent driving strategy by car quite easily.
** my default implementation called TeleportationStrategy is a single std::sleep_for a certain amount of time different for each kart and at each lap (to model a kind of Driving Skill)


## Room for improvements
* boundary cases on input parameters
* adding a parameter to read a race report
* usage of std::async or packaged_tasks instead of threads
* Would be a better design with an Event Pattern, to reduce class cloupling
* do not manage if multiple karts do the same time. Regarding the algorithm, we consider the last one as the winner.
