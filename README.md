# DSA-project
College Event Management System is a C console app for campus events. Admins manage events, participants, attendance, announcements, feedback, reports, and PDF certificates. Students create accounts, search and register for events, join waitlists, cancel registrations, and submit feedback. Data is saved in binary files for future use too.
# College Event Management System

A console-based college event management application written in C. It provides separate administrator and student menus for managing campus events, registrations, attendance, announcements, feedback, reports, and participation certificates.

## Features

### Administrator

- Add, edit, search, view, and delete events.
- Check event capacity and prevent venue/time clashes.
- View and export participant lists.
- Mark registered students as present.
- Post announcements and review student feedback.
- View an administrative summary and generate certificates for attendees.

### Student

- Create an account and log in with a registration number and password.
- Browse and search events by name or venue.
- Register for events, view personal registrations, or cancel a registration.
- Join a waitlist when an event is full. When a seat opens, the first waiting student can be promoted.
- Read announcements, change a password, submit feedback after attending, and create a personal certificate.

Certificates are generated as PDF files named `certificate_<registration-number>_<event-code>.pdf` in the program's working folder. The PDF is created by the C program without an external PDF library.

## Requirements

- A C compiler such as GCC.
- Windows, macOS, or Linux terminal.

## Build and run

From the folder containing `College_Event_Management_System.c`:

```sh
gcc -std=c99 -Wall -Wextra College_Event_Management_System.c -o event_manager
```

On Windows, run:

```powershell
.\event_manager.exe
```

On macOS or Linux, run:

```sh
./event_manager
```

The first run creates ten sample events if `events.dat` does not already exist. Choose **Student signup** from the main menu to create a student account. The administrator password in this sample project is `admin123`.

## Data files

The program stores records in binary files in its current working folder:

| File | Contents |
| --- | --- |
| `events.dat` | Event details |
| `students.dat` | Student account records |
| `registrations.dat` | Event registrations and attendance status |
| `waitlist.dat` | Students waiting for event capacity |
| `announcements.dat` | Announcement messages |
| `feedback.dat` | Student ratings and comments |

Keep running the program from the same folder to use the same data. Back up these files before deleting or changing them. The program may also create `participants_<event-code>.txt` exports and PDF certificates in that folder.

## Notes

- Student passwords are stored as plain text, and the administrator password is embedded in the source. This project is intended for learning and demonstration, not deployment with real credentials.
- The binary files contain raw C structures, so changing the structure definitions may make existing data files incompatible. Back up or remove old data files when intentionally resetting a test installation.
- Fees are event details. The program does not record payments, so its fee total is an estimate based on registrations.
- Input validation and file error handling are limited; use sample data when exploring the project.

## Project structure

```text
College_Event_Management_System.c
README.md
```
