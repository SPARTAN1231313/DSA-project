/* College Event Management System - console project in C
   "Database" = binary files: events.dat, students.dat, registrations.dat */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define EVENTS_FILE   "events.dat"
#define STUDENTS_FILE "students.dat"
#define REG_FILE      "registrations.dat"
#define WAIT_FILE     "waitlist.dat"
#define ANN_FILE      "announcements.dat"
#define FB_FILE       "feedback.dat"
#define MAX_EVENTS    200
#define ADMIN_PASS    "admin123"

typedef struct { int id; char code[10]; char name[50]; char date[11]; char time[6]; char venue[30]; int capacity; int fee; } Event;
typedef struct { char regNo[20]; char name[50]; char password[20]; } Student;
typedef struct { char regNo[20]; int eventId; int attended; } Registration;

typedef struct { char text[120]; } Announcement;
typedef struct { char regNo[20]; int eventId; int rating; char comment[80]; } Feedback;

/* ---------- input helpers ---------- */
void readLine(const char *prompt, char *buf, int size) {
    char tmp[256];
    printf("%s", prompt);
    if (!fgets(tmp, sizeof tmp, stdin)) { buf[0] = '\0'; return; }
    tmp[strcspn(tmp, "\n")] = '\0';
    strncpy(buf, tmp, size - 1);   /* safe copy: always reads the full line first */
    buf[size - 1] = '\0';
}
int readInt(const char *prompt) {
    char b[32];
    readLine(prompt, b, sizeof b);
    return atoi(b);
}
int containsIgnoreCase(const char *text, const char *word) {
    char a[100], b[100]; int i;
    for (i = 0; text[i] && i < 99; i++) { a[i] = tolower((unsigned char)text[i]); }
    a[i] = '\0';
    for (i = 0; word[i] && i < 99; i++) { b[i] = tolower((unsigned char)word[i]); }
    b[i] = '\0';
    return strstr(a, b) != NULL;
}

void toUpperStr(char *s) { for (; *s; s++) *s = toupper((unsigned char)*s); }

/* ---------- data helpers ---------- */
int countRegs(int eventId) {
    FILE *f = fopen(REG_FILE, "rb"); Registration r; int n = 0;
    if (!f) return 0;
    while (fread(&r, sizeof r, 1, f) == 1) if (r.eventId == eventId) n++;
    fclose(f);
    return n;
}
int isRegistered(const char *regNo, int eventId) {
    FILE *f = fopen(REG_FILE, "rb"); Registration r;
    if (!f) return 0;
    while (fread(&r, sizeof r, 1, f) == 1)
        if (r.eventId == eventId && strcmp(r.regNo, regNo) == 0) { fclose(f); return 1; }
    fclose(f);
    return 0;
}
int getEvent(int id, Event *out) {
    FILE *f = fopen(EVENTS_FILE, "rb"); Event e;
    if (!f) return 0;
    while (fread(&e, sizeof e, 1, f) == 1)
        if (e.id == id) { *out = e; fclose(f); return 1; }
    fclose(f);
    return 0;
}
/* asks for an event CODE (e.g. EVT101) and returns the matching event id, or 0 if not found */
int askEventId(const char *prompt) {
    char code[20]; FILE *f; Event e;
    readLine(prompt, code, sizeof code);
    toUpperStr(code);
    f = fopen(EVENTS_FILE, "rb");
    if (!f) return 0;
    while (fread(&e, sizeof e, 1, f) == 1)
        if (strcmp(e.code, code) == 0) { fclose(f); return e.id; }
    fclose(f);
    return 0;
}

/* remove registrations: regNo == NULL removes all for that event */
void removeFrom(const char *file, const char *regNo, int eventId) {
    FILE *f = fopen(file, "rb"), *t; Registration r;
    if (!f) return;
    t = fopen("tmp.dat", "wb");
    while (fread(&r, sizeof r, 1, f) == 1) {
        int match = (r.eventId == eventId) && (regNo == NULL || strcmp(r.regNo, regNo) == 0);
        if (!match) fwrite(&r, sizeof r, 1, t);
    }
    fclose(f); fclose(t);
    remove(file); rename("tmp.dat", file);
}
void removeRegs(const char *regNo, int eventId) { removeFrom(REG_FILE, regNo, eventId); }

int isWaiting(const char *regNo, int eventId) {
    FILE *f = fopen(WAIT_FILE, "rb"); Registration r;
    if (!f) return 0;
    while (fread(&r, sizeof r, 1, f) == 1)
        if (r.eventId == eventId && strcmp(r.regNo, regNo) == 0) { fclose(f); return 1; }
    fclose(f);
    return 0;
}
/* when a seat frees up, the first student on the waiting list gets it */
void promoteWaitlist(int eventId) {
    FILE *f = fopen(WAIT_FILE, "rb"); Registration w; Event e; int found = 0;
    if (!f) return;
    if (!getEvent(eventId, &e) || countRegs(eventId) >= e.capacity) { fclose(f); return; }
    while (fread(&w, sizeof w, 1, f) == 1) if (w.eventId == eventId) { found = 1; break; }
    fclose(f);
    if (!found) return;
    w.attended = 0;
    f = fopen(REG_FILE, "ab"); fwrite(&w, sizeof w, 1, f); fclose(f);
    removeFrom(WAIT_FILE, w.regNo, eventId);
    printf("Student %s from the waiting list got the free seat.\n", w.regNo);
}
int wasPresent(const char *regNo, int eventId) {
    FILE *f = fopen(REG_FILE, "rb"); Registration r;
    if (!f) return 0;
    while (fread(&r, sizeof r, 1, f) == 1)
        if (r.eventId == eventId && strcmp(r.regNo, regNo) == 0) { fclose(f); return r.attended; }
    fclose(f);
    return 0;
}
int dateKey(const char *d) { int dd = 0, mm = 0, yy = 0; sscanf(d, "%d-%d-%d", &dd, &mm, &yy); return yy * 10000 + mm * 100 + dd; }
int cmpEvents(const void *a, const void *b) {
    const Event *x = a, *y = b; int d = dateKey(x->date) - dateKey(y->date);
    return d ? d : strcmp(x->time, y->time);
}

/* ---------- 10 ready-made events (created only on the first run) ---------- */
void seedEvents(void) {
    FILE *f = fopen(EVENTS_FILE, "rb");
    Event list[10] = {
        {1,  "EVT101", "NEBULA AI Workshop",   "15-10-2026", "10:00", "Seminar Hall",     100, 0},
        {2,  "EVT102", "Hackathon 2026",       "16-10-2026", "09:00", "Main Auditorium",  200, 100},
        {3,  "EVT103", "Cultural Fest Night",  "17-10-2026", "17:00", "Open Air Theatre", 500, 50},
        {4,  "EVT104", "Cricket Tournament",   "18-10-2026", "08:00", "Sports Ground",    150, 0},
        {5,  "EVT105", "Coding Contest",       "19-10-2026", "11:00", "Lab 101",           60, 0},
        {6,  "EVT106", "Robotics Expo",        "20-10-2026", "10:00", "Main Auditorium",  120, 0},
        {7,  "EVT107", "Quiz Competition",     "21-10-2026", "14:00", "Seminar Hall",      80, 20},
        {8,  "EVT108", "Photography Walk",     "22-10-2026", "07:30", "Campus Garden",     40, 0},
        {9,  "EVT109", "Tech Talk: Cloud",     "23-10-2026", "15:00", "Seminar Hall",     100, 0},
        {10, "EVT110", "Farewell Party",       "25-10-2026", "18:00", "Open Air Theatre", 300, 150}
    };
    if (f) { fclose(f); return; }      /* file already exists, keep user data */
    f = fopen(EVENTS_FILE, "wb");
    fwrite(list, sizeof(Event), 10, f);
    fclose(f);
}

/* ---------- events ---------- */
void printEvent(const Event *e) {
    printf("%-7s %-22s %-11s %-6s %-17s %-5d Rs.%-4d %d left\n", e->code, e->name, e->date, e->time,
           e->venue, e->capacity, e->fee, e->capacity - countRegs(e->id));
}
void viewEvents(const char *keyword) {
    static Event list[MAX_EVENTS]; int n = 0, i, found = 0;
    FILE *f = fopen(EVENTS_FILE, "rb");
    if (f) { while (n < MAX_EVENTS && fread(&list[n], sizeof(Event), 1, f) == 1) n++; fclose(f); }
    qsort(list, n, sizeof(Event), cmpEvents);          /* sorted by date, then time */
    printf("\n%-7s %-22s %-11s %-6s %-17s %-5s %-7s %s\n", "Code", "Name", "Date", "Time", "Venue", "Seats", "Fee", "Left");
    printf("--------------------------------------------------------------------------------------------\n");
    for (i = 0; i < n; i++)
        if (!keyword || containsIgnoreCase(list[i].name, keyword) || containsIgnoreCase(list[i].venue, keyword)) { printEvent(&list[i]); found = 1; }
    if (!found) printf("No events found.\n");
}
void addEvent(void) {
    Event e, x; FILE *f;
    readLine("Event code (e.g. EVT111): ", e.code, sizeof e.code);
    toUpperStr(e.code);
    readLine("Event name: ", e.name, sizeof e.name);
    readLine("Date (DD-MM-YYYY): ", e.date, sizeof e.date);
    readLine("Time (HH:MM): ", e.time, sizeof e.time);
    readLine("Venue: ", e.venue, sizeof e.venue);
    e.capacity = readInt("Capacity: ");
    e.fee = readInt("Fee (0 if free): ");
    if (e.capacity <= 0 || strlen(e.name) == 0 || strlen(e.code) == 0) { printf("Invalid details.\n"); return; }
    e.id = 1;
    f = fopen(EVENTS_FILE, "rb");
    if (f) {
        while (fread(&x, sizeof x, 1, f) == 1) {
            if (x.id >= e.id) e.id = x.id + 1;
            if (strcmp(x.code, e.code) == 0) { printf("Event code %s already exists.\n", e.code); fclose(f); return; }
            if (strcmp(x.venue, e.venue) == 0 && strcmp(x.date, e.date) == 0 && strcmp(x.time, e.time) == 0) {
                printf("Venue clash with \"%s\". Event not added.\n", x.name);
                fclose(f); return;
            }
        }
        fclose(f);
    }
    f = fopen(EVENTS_FILE, "ab");
    fwrite(&e, sizeof e, 1, f);
    fclose(f);
    printf("Event %s added successfully.\n", e.code);
}
void deleteEvent(void) {
    int id = askEventId("Event code to delete: "), found = 0;
    FILE *f = fopen(EVENTS_FILE, "rb"), *t; Event e;
    if (!f) { printf("No events.\n"); return; }
    t = fopen("tmp.dat", "wb");
    while (fread(&e, sizeof e, 1, f) == 1) { if (e.id == id) found = 1; else fwrite(&e, sizeof e, 1, t); }
    fclose(f); fclose(t);
    remove(EVENTS_FILE); rename("tmp.dat", EVENTS_FILE);
    if (found) { removeRegs(NULL, id); removeFrom(WAIT_FILE, NULL, id); printf("Event deleted.\n"); } else printf("Event not found.\n");
}
/* prints participants to any stream (screen or file) */
int writeParticipants(FILE *out, const Event *e) {
    Registration r; Student s; FILE *f, *sf; int n = 0;
    fprintf(out, "\nParticipants for %s [%s] (%d/%d)\n", e->name, e->code, countRegs(e->id), e->capacity);
    f = fopen(REG_FILE, "rb");
    if (f) {
        while (fread(&r, sizeof r, 1, f) == 1) {
            if (r.eventId != e->id) continue;
            sf = fopen(STUDENTS_FILE, "rb");
            if (sf) {
                while (fread(&s, sizeof s, 1, sf) == 1)
                    if (strcmp(s.regNo, r.regNo) == 0) {
                        fprintf(out, "%d. %s (%s) - %s\n", ++n, s.name, s.regNo, r.attended ? "Present" : "Absent");
                        break;
                    }
                fclose(sf);
            }
        }
        fclose(f);
    }
    if (!n) fprintf(out, "No registrations yet.\n");
    return n;
}
void viewParticipants(void) {
    Event e;
    if (!getEvent(askEventId("Event code: "), &e)) { printf("Event not found.\n"); return; }
    writeParticipants(stdout, &e);
}
void exportParticipants(void) {
    Event e; char name[40]; FILE *out;
    if (!getEvent(askEventId("Event code to export: "), &e)) { printf("Event not found.\n"); return; }
    sprintf(name, "participants_%s.txt", e.code);
    out = fopen(name, "w");
    writeParticipants(out, &e);
    fclose(out);
    printf("Saved to %s\n", name);
}
void markAttendance(void) {
    Event e; Registration r; char reg[20]; FILE *f; int done = 0;
    if (!getEvent(askEventId("Event code: "), &e)) { printf("Event not found.\n"); return; }
    if (!writeParticipants(stdout, &e)) return;
    readLine("Reg no to mark PRESENT: ", reg, sizeof reg);
    f = fopen(REG_FILE, "r+b");
    while (f && fread(&r, sizeof r, 1, f) == 1)
        if (r.eventId == e.id && strcmp(r.regNo, reg) == 0) {
            r.attended = 1;
            fseek(f, -(long)sizeof r, SEEK_CUR);
            fwrite(&r, sizeof r, 1, f);
            done = 1; break;
        }
    if (f) fclose(f);
    printf(done ? "Marked present.\n" : "Student not registered for this event.\n");
}
void setIfNotEmpty(char *dest, int size, const char *src) {
    if (src[0]) { strncpy(dest, src, size - 1); dest[size - 1] = '\0'; }
}
int hasClash(const Event *e) {
    FILE *f = fopen(EVENTS_FILE, "rb"); Event x;
    if (!f) return 0;
    while (fread(&x, sizeof x, 1, f) == 1)
        if (x.id != e->id && strcmp(x.venue, e->venue) == 0 && strcmp(x.date, e->date) == 0 && strcmp(x.time, e->time) == 0) { fclose(f); return 1; }
    fclose(f);
    return 0;
}
void editEvent(void) {
    int id = askEventId("Event code to edit: "), idx = 0, found = 0;
    Event e; char buf[60]; FILE *f = fopen(EVENTS_FILE, "rb");
    if (!f) { printf("No events.\n"); return; }
    while (fread(&e, sizeof e, 1, f) == 1) { if (e.id == id) { found = 1; break; } idx++; }
    fclose(f);
    if (!found) { printf("Event not found.\n"); return; }
    printf("Editing %s (press Enter to keep the current value)\n", e.name);
    readLine("New name: ", buf, sizeof buf);              setIfNotEmpty(e.name, sizeof e.name, buf);
    readLine("New date (DD-MM-YYYY): ", buf, sizeof buf); setIfNotEmpty(e.date, sizeof e.date, buf);
    readLine("New time (HH:MM): ", buf, sizeof buf);      setIfNotEmpty(e.time, sizeof e.time, buf);
    readLine("New venue: ", buf, sizeof buf);             setIfNotEmpty(e.venue, sizeof e.venue, buf);
    readLine("New capacity: ", buf, sizeof buf);          if (atoi(buf) > 0) e.capacity = atoi(buf);
    readLine("New fee: ", buf, sizeof buf);               if (buf[0]) e.fee = atoi(buf);
    if (e.capacity < countRegs(id)) { printf("Capacity can't be below current registrations (%d). Not saved.\n", countRegs(id)); return; }
    if (hasClash(&e)) { printf("Venue clash with another event. Not saved.\n"); return; }
    f = fopen(EVENTS_FILE, "r+b");
    fseek(f, (long)idx * sizeof(Event), SEEK_SET);
    fwrite(&e, sizeof e, 1, f);
    fclose(f);
    printf("Event updated.\n");
}
void showReport(void) {
    FILE *f; Event e; Student s; Registration r;
    int events = 0, students = 0, regs = 0, present = 0, best = -1, n; long revenue = 0; char top[50] = "-";
    f = fopen(EVENTS_FILE, "rb");
    if (f) {
        while (fread(&e, sizeof e, 1, f) == 1) {
            events++; n = countRegs(e.id); revenue += (long)n * e.fee;
            if (n > best) { best = n; strcpy(top, e.name); }
        }
        fclose(f);
    }
    f = fopen(STUDENTS_FILE, "rb");
    if (f) { while (fread(&s, sizeof s, 1, f) == 1) students++; fclose(f); }
    f = fopen(REG_FILE, "rb");
    if (f) { while (fread(&r, sizeof r, 1, f) == 1) { regs++; if (r.attended) present++; } fclose(f); }
    printf("\n====== REPORT ======\n");
    printf("Total events        : %d\n", events);
    printf("Total students      : %d\n", students);
    printf("Total registrations : %d\n", regs);
    printf("Marked present      : %d\n", present);
    printf("Fees collected (est): Rs.%ld\n", revenue);
    printf("Most popular event  : %s (%d registrations)\n", top, best < 0 ? 0 : best);
}

/* ---------- students ---------- */
void signup(void) {
    Student s, x; FILE *f;
    readLine("Registration no: ", s.regNo, sizeof s.regNo);
    readLine("Name: ", s.name, sizeof s.name);
    readLine("Password: ", s.password, sizeof s.password);
    if (strlen(s.regNo) == 0 || strlen(s.password) < 4) { printf("Invalid (password min 4 chars).\n"); return; }
    f = fopen(STUDENTS_FILE, "rb");
    if (f) {
        while (fread(&x, sizeof x, 1, f) == 1)
            if (strcmp(x.regNo, s.regNo) == 0) { printf("Already registered.\n"); fclose(f); return; }
        fclose(f);
    }
    f = fopen(STUDENTS_FILE, "ab");
    fwrite(&s, sizeof s, 1, f);
    fclose(f);
    printf("Signup successful. You can log in now.\n");
}
int studentLogin(Student *out) {
    char reg[20], pass[20]; Student s; FILE *f;
    readLine("Registration no: ", reg, sizeof reg);
    readLine("Password: ", pass, sizeof pass);
    f = fopen(STUDENTS_FILE, "rb");
    if (!f) return 0;
    while (fread(&s, sizeof s, 1, f) == 1)
        if (strcmp(s.regNo, reg) == 0 && strcmp(s.password, pass) == 0) { *out = s; fclose(f); return 1; }
    fclose(f);
    return 0;
}
void registerForEvent(const Student *s) {
    int id = askEventId("Event code to register (e.g. EVT101): "); Event e; Registration r; FILE *f;
    if (!getEvent(id, &e)) { printf("Event not found.\n"); return; }
    if (isRegistered(s->regNo, id)) { printf("You are already registered.\n"); return; }
    if (countRegs(id) >= e.capacity) {
        char a[10];
        if (isWaiting(s->regNo, id)) { printf("You are already on the waiting list.\n"); return; }
        readLine("Event is full. Join the waiting list? (y/n): ", a, sizeof a);
        if (a[0] == 'y' || a[0] == 'Y') {
            Registration w; strcpy(w.regNo, s->regNo); w.eventId = id; w.attended = 0;
            f = fopen(WAIT_FILE, "ab"); fwrite(&w, sizeof w, 1, f); fclose(f);
            printf("Added to the waiting list.\n");
        }
        return;
    }
    strcpy(r.regNo, s->regNo); r.eventId = id; r.attended = 0;
    f = fopen(REG_FILE, "ab");
    fwrite(&r, sizeof r, 1, f);
    fclose(f);
    printf("Registered for %s!\n", e.name);
}
void myEvents(const Student *s) {
    FILE *f = fopen(REG_FILE, "rb"); Registration r; Event e; int n = 0;
    printf("\nYour registered events:\n");
    if (f) {
        while (fread(&r, sizeof r, 1, f) == 1)
            if (strcmp(r.regNo, s->regNo) == 0 && getEvent(r.eventId, &e)) { printf("%d. [%s] %s - %s %s at %s%s\n", ++n, e.code, e.name, e.date, e.time, e.venue, r.attended ? " (Attended)" : ""); }
        fclose(f);
    }
    if (!n) printf("None yet.\n");
}

/* ---------- announcements ---------- */
void postAnnouncement(void) {
    Announcement a; FILE *f;
    readLine("Announcement: ", a.text, sizeof a.text);
    if (!a.text[0]) { printf("Empty message.\n"); return; }
    f = fopen(ANN_FILE, "ab"); fwrite(&a, sizeof a, 1, f); fclose(f);
    printf("Announcement posted.\n");
}
void showAnnouncements(void) {
    FILE *f = fopen(ANN_FILE, "rb"); Announcement a; int total = 0, i = 0;
    if (!f) { printf("\nNo announcements.\n"); return; }
    while (fread(&a, sizeof a, 1, f) == 1) total++;
    rewind(f);
    printf("\n--- ANNOUNCEMENTS (latest 5) ---\n");
    while (fread(&a, sizeof a, 1, f) == 1) if (i++ >= total - 5) printf("* %s\n", a.text);
    fclose(f);
    if (!total) printf("No announcements.\n");
}

/* ---------- feedback ---------- */
void giveFeedback(const Student *s) {
    Event e; Feedback fb, x; FILE *f;
    if (!getEvent(askEventId("Event code: "), &e)) { printf("Event not found.\n"); return; }
    if (!wasPresent(s->regNo, e.id)) { printf("Feedback is only for events you attended.\n"); return; }
    f = fopen(FB_FILE, "rb");
    if (f) {
        while (fread(&x, sizeof x, 1, f) == 1)
            if (x.eventId == e.id && strcmp(x.regNo, s->regNo) == 0) { printf("You already gave feedback.\n"); fclose(f); return; }
        fclose(f);
    }
    fb.rating = readInt("Rating (1-5): ");
    if (fb.rating < 1 || fb.rating > 5) { printf("Rating must be 1 to 5.\n"); return; }
    readLine("Comment: ", fb.comment, sizeof fb.comment);
    strcpy(fb.regNo, s->regNo); fb.eventId = e.id;
    f = fopen(FB_FILE, "ab"); fwrite(&fb, sizeof fb, 1, f); fclose(f);
    printf("Thanks for your feedback!\n");
}
void viewFeedback(void) {
    Event e; Feedback x; FILE *f; int n = 0, sum = 0;
    if (!getEvent(askEventId("Event code: "), &e)) { printf("Event not found.\n"); return; }
    printf("\nFeedback for %s\n", e.name);
    f = fopen(FB_FILE, "rb");
    if (f) {
        while (fread(&x, sizeof x, 1, f) == 1)
            if (x.eventId == e.id) { printf("%s: %d/5 - %s\n", x.regNo, x.rating, x.comment); n++; sum += x.rating; }
        fclose(f);
    }
    if (n) printf("Average rating: %.1f/5 (%d responses)\n", (float)sum / n, n); else printf("No feedback yet.\n");
}

/* ---------- certificates ---------- */
/* Add centered, escaped ASCII text to a one-page PDF content stream. */
void pdfCenteredText(char *content, size_t contentSize, int y, int fontSize,
                     const char *text) {
    char safe[256];
    size_t i, j = 0;
    int x;
    for (i = 0; text[i] && j < sizeof safe - 2; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c < 32) continue;
        if (c > 126) c = '?'; /* built-in PDF fonts use a limited character set */
        if (c == '(' || c == ')' || c == '\\') safe[j++] = '\\';
        safe[j++] = (char)c;
    }
    safe[j] = '\0';
    x = (612 - (int)j * fontSize / 2) / 2;
    snprintf(content + strlen(content), contentSize - strlen(content),
             "BT /F1 %d Tf 1 0 0 1 %d %d Tm (%s) Tj ET\n",
             fontSize, x, y, safe);
}

void writeCertificate(const char *name, const char *regNo, const Event *e) {
    char fn[100], content[4096] = "";
    char safeReg[20], safeCode[10];
    FILE *f;
    long offsets[6], xrefOffset;
    int i;

    /* Keep user-provided identifiers from becoming path components. */
    for (i = 0; regNo[i] && i < (int)sizeof safeReg - 1; i++)
        safeReg[i] = (isalnum((unsigned char)regNo[i]) || regNo[i] == '-' || regNo[i] == '_') ? regNo[i] : '_';
    safeReg[i] = '\0';
    for (i = 0; e->code[i] && i < (int)sizeof safeCode - 1; i++)
        safeCode[i] = (isalnum((unsigned char)e->code[i]) || e->code[i] == '-' || e->code[i] == '_') ? e->code[i] : '_';
    safeCode[i] = '\0';
    snprintf(fn, sizeof fn, "certificate_%s_%s.pdf", safeReg, safeCode);

    /* Draw a simple framed certificate, then place its text. */
    snprintf(content, sizeof content,
             "q\n0.97 0.98 0.99 rg 0 0 612 792 re f\nQ\n"
             "0.08 0.25 0.38 RG 2.5 w 28 28 556 736 re S\n"
             "0.72 0.52 0.20 RG 1 w 42 42 528 708 re S\n");
    pdfCenteredText(content, sizeof content, 660, 13, "COLLEGE EVENT MANAGEMENT SYSTEM");
    pdfCenteredText(content, sizeof content, 585, 27, "CERTIFICATE OF PARTICIPATION");
    pdfCenteredText(content, sizeof content, 510, 14, "This is to certify that");
    pdfCenteredText(content, sizeof content, 455, 23, name);
    {
        char line[80];
        snprintf(line, sizeof line, "Registration number: %s", regNo);
        pdfCenteredText(content, sizeof content, 420, 12, line);
    }
    pdfCenteredText(content, sizeof content, 360, 14, "has successfully participated in");
    pdfCenteredText(content, sizeof content, 315, 21, e->name);
    {
        char line[120];
        snprintf(line, sizeof line, "Held on %s at %s", e->date, e->venue);
        pdfCenteredText(content, sizeof content, 270, 12, line);
    }
    pdfCenteredText(content, sizeof content, 110, 10, "Issued by the College Event Management System");

    f = fopen(fn, "wb");
    if (!f) { printf("Could not create %s\n", fn); return; }

    fputs("%PDF-1.4\n", f);
    offsets[1] = ftell(f);
    fputs("1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj\n", f);
    offsets[2] = ftell(f);
    fputs("2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\n", f);
    offsets[3] = ftell(f);
    fputs("3 0 obj << /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >> endobj\n", f);
    offsets[4] = ftell(f);
    fputs("4 0 obj << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> endobj\n", f);
    offsets[5] = ftell(f);
    fprintf(f, "5 0 obj << /Length %u >> stream\n%sendstream\nendobj\n",
            (unsigned)strlen(content), content);

    xrefOffset = ftell(f);
    fputs("xref\n0 6\n0000000000 65535 f \n", f);
    for (i = 1; i <= 5; i++) fprintf(f, "%010ld 00000 n \n", offsets[i]);
    fprintf(f, "trailer << /Size 6 /Root 1 0 R >>\nstartxref\n%ld\n%%%%EOF\n", xrefOffset);
    fclose(f);
    printf("PDF certificate saved: %s\n", fn);
}
void myCertificate(const Student *s) {
    Event e;
    if (!getEvent(askEventId("Event code: "), &e)) { printf("Event not found.\n"); return; }
    if (!wasPresent(s->regNo, e.id)) { printf("Certificates are only for events you attended.\n"); return; }
    writeCertificate(s->name, s->regNo, &e);
}
void generateCertificates(void) {
    Event e; Registration r; Student st; FILE *f, *sf; int n = 0;
    if (!getEvent(askEventId("Event code: "), &e)) { printf("Event not found.\n"); return; }
    f = fopen(REG_FILE, "rb");
    while (f && fread(&r, sizeof r, 1, f) == 1) {
        if (r.eventId != e.id || !r.attended) continue;
        sf = fopen(STUDENTS_FILE, "rb");
        if (sf) {
            while (fread(&st, sizeof st, 1, sf) == 1)
                if (strcmp(st.regNo, r.regNo) == 0) { writeCertificate(st.name, st.regNo, &e); n++; break; }
            fclose(sf);
        }
    }
    if (f) fclose(f);
    printf("%d certificate(s) generated.\n", n);
}

void changePassword(Student *s) {
    char old[20], np[20]; FILE *f; Student x;
    readLine("Current password: ", old, sizeof old);
    if (strcmp(old, s->password) != 0) { printf("Wrong current password.\n"); return; }
    readLine("New password (min 4 chars): ", np, sizeof np);
    if (strlen(np) < 4) { printf("Too short.\n"); return; }
    f = fopen(STUDENTS_FILE, "r+b");
    if (!f) return;
    while (fread(&x, sizeof x, 1, f) == 1)
        if (strcmp(x.regNo, s->regNo) == 0) {
            strcpy(x.password, np);
            fseek(f, -(long)sizeof x, SEEK_CUR);
            fwrite(&x, sizeof x, 1, f);
            break;
        }
    fclose(f);
    strcpy(s->password, np);
    printf("Password changed.\n");
}

/* ---------- menus ---------- */
void adminMenu(void) {
    int c;
    do {
        printf("\n--- ADMIN MENU ---\n1. Add event\n2. View events\n3. Search events\n4. Edit event\n5. Delete event\n"
               "6. View participants\n7. Mark attendance\n8. Export participants to file\n9. Report\n10. Post announcement\n11. View feedback\n12. Generate certificates\n0. Logout\n");
        c = readInt("Choice: ");
        if (c == 1) addEvent();
        else if (c == 2) viewEvents(NULL);
        else if (c == 3) { char k[40]; readLine("Search keyword: ", k, sizeof k); viewEvents(k); }
        else if (c == 4) editEvent();
        else if (c == 5) deleteEvent();
        else if (c == 6) viewParticipants();
        else if (c == 7) markAttendance();
        else if (c == 8) exportParticipants();
        else if (c == 9) showReport();
        else if (c == 10) postAnnouncement();
        else if (c == 11) viewFeedback();
        else if (c == 12) generateCertificates();
    } while (c != 0);
}
void studentMenu(Student *s) {
    int c;
    printf("\nWelcome, %s!\n", s->name);
    showAnnouncements();
    do {
        printf("\n--- STUDENT MENU ---\n1. View events\n2. Search events\n3. Register for event\n4. My events\n5. Cancel registration\n6. Change password\n7. Announcements\n8. Give feedback\n9. My certificate\n0. Logout\n");
        c = readInt("Choice: ");
        if (c == 1) viewEvents(NULL);
        else if (c == 2) { char k[40]; readLine("Search keyword: ", k, sizeof k); viewEvents(k); }
        else if (c == 3) registerForEvent(s);
        else if (c == 4) myEvents(s);
        else if (c == 5) {
            int id = askEventId("Event code to cancel: ");
            if (isRegistered(s->regNo, id)) { removeRegs(s->regNo, id); printf("Registration cancelled.\n"); promoteWaitlist(id); }
            else if (isWaiting(s->regNo, id)) { removeFrom(WAIT_FILE, s->regNo, id); printf("Removed from the waiting list.\n"); }
            else printf("You are not registered for that event.\n");
        }
        else if (c == 6) changePassword(s);
        else if (c == 7) showAnnouncements();
        else if (c == 8) giveFeedback(s);
        else if (c == 9) myCertificate(s);
    } while (c != 0);
}

int main(void) {
    int c;
    seedEvents();
    do {
        printf("\n===== COLLEGE EVENT MANAGEMENT =====\n1. Admin login\n2. Student login\n3. Student signup\n0. Exit\n");
        c = readInt("Choice: ");
        if (c == 1) {
            char p[30]; readLine("Admin password: ", p, sizeof p);
            if (strcmp(p, ADMIN_PASS) == 0) adminMenu(); else printf("Wrong password.\n");
        } else if (c == 2) {
            Student s;
            if (studentLogin(&s)) studentMenu(&s); else printf("Invalid login.\n");
        } else if (c == 3) signup();
    } while (c != 0);
    printf("Goodbye!\n");
    return 0;
}
