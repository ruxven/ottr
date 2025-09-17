# O T T R

*O*ne *T*enth *T*ime *R*eporter

## What is this?

A tool for tracking time for timecard charging,
especially if instructed to charge to multiple charge codes with varying percentages (for example, a a 65%/35% split).

It is intended to work with time increments no smaller than the _tenth_ of an hour,
which is common in contracting and time charging systems.

## How do I use it?

TL;DR: `ottr ./demo_timecard.txt 09/02`

Have fun :)

## Usage: No really, how do I use it?

`ottr` uses a plain text file that defines:

* Charge Numbers
* Tasks
* Association of Charge Numbers with Tasks
* Day entries
* Log entries for each day, starting a new task or ending one.

Log entries reference tasks which are associated with a preceding Day entry.
Association of charge numbers with tasks requires the defintion of those charge numbers and tasks.

When writing a timecard file, first define the charge numbers in your supported projects:

```
# cn (charge code ID) (description) (hours available) (optional weight)
cn 1234.01 "Project A" 99.0
cn 1234.02 "Project B" 99.0 1
```

The charge code ID is the unique identifier used by your time charge system (Note this tool does not interface with any time charge system, it is strictly for reference purposes only).
Hours available can be tricky if that information isn't available, so use best judgement.
If in doubt, use the number of hours you would theoretically work on it.
The optional weight is useful when the tool is deciding where to assign remainder hour segments (I like to call them _decihours_).

Next entries to follow are tasks.
These relate to activities that can be associated with the charge number which can be as detailed or generic as desired.

```
# task (name) (description)
task meeting "Meetings"
task design_review "Design Review for Project B"
task lunch "Lunch"
```

The name of the task is referenced in the log entries further on.
Note any activity can be documented here as a task, even if it isn't associated with any charge codes.

With tasks and charge code defined, now their associations and weights can be defined:

```
# wt (task_name) (cn_id) (value)
wt meeting 1234.01 50
wt meeting 1234.02 50

wt design_review 1234.02 
```

By not associating the `lunch` task with any charge number, it is effectively an _uncharged task_ which can be useful
(for example, tracking how much time you had to listen to a coworker talk about that ridiculous display last night!).

The weights can be any integer value, which are summed up and used to divide the charge time accross the associated tasks.

Now for day entries. OTTR has no concept of when the timecard period starts or ends, except for the first day entry and the last day entry (which could be the same day!).

```
# day MM/DD
day 09/02
```

There is no day terminator. The last day of the timecard period is the last day entry processed in the file.

With a day entry, now we can finally add the log entries for that day:

```
day 09/02
# log (hour) (task) (optional details)
log 08.5 meeting "Morning Tag Up"
log 09.0 design_review
log 09.2 meeting "Client call"
log 09.5 design_review
log 10.0 meeting "Weekly Status Report"
log 11.0 design_review
log 11.2 nc "watercooler chat"
log 11.7 design_review
log 11.8 lunch "give up, goto lunch"
log 13.0 design_review
log 14.0 meeting "Afternoon Status Report"
log 15.0 design_review
log 15.2 nc "followup watercooler chat"
log 16.0 design_review
log 17.0 nc "Have to leave to make it home in time for dinner"
```

Hours are tracked using hours accurate to the _tenth_ of an hour (i.e. 6 minutes).
Consult your company's documentation for how that should be documented (for example, does coming in at 9:01 count as 9.1 or 9.0?).

This is only as accurate as you need it to be, so use your best judgement.

Note the `nc` entry, which doesn't have an associated task name.
In this case, this is an *uncharged task* that can be used to document events that don't require a task entry.

Finally with all of the necessary timecard info entered, the `ottr` tool can be used:

```
ottr my_timecard-2025.09.01.txt
```

Which will output a table of values, organized by charge code id and day:

```
| Charge  | 09/02 | 09/03 | 09/04 |  Rem |
| 1234.01 |   4.2 |   2.3 |   6.3 | 99.0 |
| 1234.02 |   0.4 |   1.2 |   2.2 | 80 0 |
| lunch   |   1.2 |   0.5 |   1.0 |  0.0 |
| Total   |   6.0 |   4.0 |   9.5 |      |
```

Or to view the values of a specific day:

```
ottr my_timecard-2025.09.01.txt 09/03
```

```
| Charge  | 09/03 |
| 1234.01 |   2.3 |
| 1234.02 |   1.2 |
| lunch   |   0.5 |
| Total   |   4.0 |
```

## Building

`ottr` is built using CMake and can be installed locally:

```
mkdir build
cd build
cmake ..
make
cp ottr ~/.local/bin/ottr
```

## TODO

- Add configuration options
- Summarize task data charges per day or per event.
- Generate a timecard using a specified date range.
- Audit mode: show the per-day allocation and remainder assignments per event or per day.
- Add a command to directly add time to a charge code, as a feature of last resort.
