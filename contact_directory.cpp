/*
 * Project 11: File-Based Contact Directory
 * Subject : Object-Oriented Programming using C++ (CIC-211)
 *
 * Classes
 *   Contact          - one contact (private data, validation in setters)
 *   ContactDirectory - collection of contacts + file handling + operations
 *
 * Data file format (contacts.txt), one contact per line:
 *   name|phone|email
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <cstdlib>
using namespace std;

// ---------------------------------------------------------------- helpers
static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static string toLower(string s) {
    for (char& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

// Prints a message, reads one full line, trims it.
static string readLine(const string& msg) {
    cout << msg;
    string s;
    if (!getline(cin, s)) {              // input stream closed (Ctrl+D / Ctrl+Z)
        cout << "\nInput closed. Exiting.\n";
        exit(0);                         // data is already saved after every change
    }
    return trim(s);
}

// ------------------------------------------------------------- validation
namespace Validator {
    bool validName(const string& n) {
        return !n.empty() && n.find('|') == string::npos;
    }
    // 10-13 digits, optional leading '+'
    bool validPhone(const string& p) {
        size_t start = (!p.empty() && p[0] == '+') ? 1 : 0;
        size_t digits = p.size() - start;
        if (digits < 10 || digits > 13) return false;
        for (size_t i = start; i < p.size(); ++i)
            if (!isdigit(static_cast<unsigned char>(p[i]))) return false;
        return true;
    }
    bool validEmail(const string& e) {
        if (e.find('|') != string::npos || e.find(' ') != string::npos) return false;
        size_t at = e.find('@');
        if (at == string::npos || at == 0) return false;
        size_t dot = e.find('.', at + 1);
        return dot != string::npos && dot > at + 1 && dot + 1 < e.size();
    }
}

// ---------------------------------------------------------------- Contact
class Contact {
private:
    string name;
    string phone;   // unique key
    string email;

public:
    Contact() {}
    Contact(const string& n, const string& p, const string& e) {
        setName(n);
        setPhone(p);
        setEmail(e);
    }

    // getters
    const string& getName()  const { return name; }
    const string& getPhone() const { return phone; }
    const string& getEmail() const { return email; }

    // setters (validate and throw on bad data)
    void setName(const string& n) {
        if (!Validator::validName(n))
            throw invalid_argument("Name cannot be empty or contain '|'.");
        name = n;
    }
    void setPhone(const string& p) {
        if (!Validator::validPhone(p))
            throw invalid_argument("Phone must have 10-13 digits (optional leading +).");
        phone = p;
    }
    void setEmail(const string& e) {
        if (!Validator::validEmail(e))
            throw invalid_argument("Email must look like name@domain.com (no spaces).");
        email = e;
    }

    string toFileLine() const { return name + "|" + phone + "|" + email; }

    static Contact fromFileLine(const string& line) {
        vector<string> parts;
        string part;
        stringstream ss(line);
        while (getline(ss, part, '|')) parts.push_back(part);
        if (parts.size() != 3) throw runtime_error("Malformed record: " + line);
        return Contact(parts[0], parts[1], parts[2]);
    }

    void display(int no) const {
        cout << left << setw(4) << no << setw(24) << name
             << setw(16) << phone << email << "\n";
    }
};

// -------------------------------------------------------- ContactDirectory
class ContactDirectory {
private:
    string filename;
    vector<Contact> contacts;

    vector<Contact>::iterator findByPhone(const string& phone) {
        return find_if(contacts.begin(), contacts.end(),
                       [&](const Contact& c) { return c.getPhone() == phone; });
    }

    static void printHeader() {
        cout << left << setw(4) << "No." << setw(24) << "Name"
             << setw(16) << "Phone" << "Email\n";
        cout << string(64, '-') << "\n";
    }

    void save() const {
        ofstream out(filename.c_str());
        if (!out) throw runtime_error("Cannot open '" + filename + "' for writing.");
        for (vector<Contact>::const_iterator it = contacts.begin(); it != contacts.end(); ++it)
            out << it->toFileLine() << "\n";
    }

public:
    explicit ContactDirectory(const string& file) : filename(file) {}

    // Load contacts at start-up; a missing file simply means an empty directory.
    void load() {
        ifstream in(filename.c_str());
        if (!in) {
            cout << "No data file found. Starting with an empty directory.\n";
            return;
        }
        string line;
        int skipped = 0, loaded = 0;
        while (getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            try {
                Contact c = Contact::fromFileLine(line);
                if (findByPhone(c.getPhone()) != contacts.end()) { ++skipped; continue; }
                contacts.push_back(c);
                ++loaded;
            } catch (const exception&) {
                ++skipped;               // ignore corrupt lines
            }
        }
        cout << loaded << " contact(s) loaded.";
        if (skipped) cout << " " << skipped << " invalid/duplicate line(s) skipped.";
        cout << "\n";
    }

    void addContact() {
        string n = readLine("Name  : ");
        string p = readLine("Phone : ");
        string e = readLine("Email : ");
        Contact c(n, p, e);                          // may throw invalid_argument
        if (findByPhone(c.getPhone()) != contacts.end())
            throw invalid_argument("A contact with this phone number already exists.");
        contacts.push_back(c);
        save();
        cout << "Contact added and saved.\n";
    }

    void displayAll() const {
        if (contacts.empty()) { cout << "Directory is empty.\n"; return; }
        vector<Contact> sorted(contacts);            // show alphabetically
        sort(sorted.begin(), sorted.end(), [](const Contact& a, const Contact& b) {
            return toLower(a.getName()) < toLower(b.getName());
        });
        printHeader();
        int i = 1;
        for (vector<Contact>::const_iterator it = sorted.begin(); it != sorted.end(); ++it)
            it->display(i++);
        cout << "Total: " << sorted.size() << " contact(s)\n";
    }

    // mode 1 = name (partial, case-insensitive), mode 2 = phone (partial)
    void search(int mode) const {
        string key = readLine(mode == 1 ? "Enter name to search : " : "Enter phone to search: ");
        if (key.empty()) throw invalid_argument("Search text cannot be empty.");
        string lkey = toLower(key);
        int found = 0;
        for (vector<Contact>::const_iterator it = contacts.begin(); it != contacts.end(); ++it) {
            bool match = (mode == 1)
                ? toLower(it->getName()).find(lkey) != string::npos
                : it->getPhone().find(key) != string::npos;
            if (match) {
                if (found == 0) printHeader();
                it->display(++found);
            }
        }
        if (found == 0) cout << "No matching contact found.\n";
    }

    void editContact() {
        string key = readLine("Phone number of the contact to edit: ");
        vector<Contact>::iterator it = findByPhone(key);
        if (it == contacts.end()) { cout << "Contact not found.\n"; return; }

        cout << "Leave a field blank to keep the current value.\n";
        string n = readLine("New name  [" + it->getName()  + "]: ");
        string p = readLine("New phone [" + it->getPhone() + "]: ");
        string e = readLine("New email [" + it->getEmail() + "]: ");

        Contact updated = *it;                       // work on a copy: all-or-nothing
        if (!n.empty()) updated.setName(n);
        if (!p.empty()) {
            updated.setPhone(p);
            if (p != it->getPhone() && findByPhone(p) != contacts.end())
                throw invalid_argument("Another contact already uses this phone number.");
        }
        if (!e.empty()) updated.setEmail(e);

        *it = updated;
        save();
        cout << "Contact updated and saved.\n";
    }

    void deleteContact() {
        string key = readLine("Phone number of the contact to delete: ");
        vector<Contact>::iterator it = findByPhone(key);
        if (it == contacts.end()) { cout << "Contact not found.\n"; return; }
        string ans = readLine("Delete '" + it->getName() + "'? (y/n): ");
        if (toLower(ans) != "y") { cout << "Deletion cancelled.\n"; return; }
        contacts.erase(it);
        save();
        cout << "Contact deleted and saved.\n";
    }
};

// ------------------------------------------------------------------- main
static void showMenu() {
    cout << "\n===== CONTACT DIRECTORY =====\n"
         << "1. Add contact\n"
         << "2. Display all contacts\n"
         << "3. Search by name\n"
         << "4. Search by phone\n"
         << "5. Edit contact\n"
         << "6. Delete contact\n"
         << "0. Exit\n";
}

int main() {
    ContactDirectory dir("contacts.txt");
    dir.load();

    int choice = -1;
    do {
        showMenu();
        string input = readLine("Enter choice: ");
        try {
            choice = stoi(input);                    // throws on non-numeric input
        } catch (const exception&) {
            choice = -1;
        }

        try {
            switch (choice) {
                case 1: dir.addContact();    break;
                case 2: dir.displayAll();    break;
                case 3: dir.search(1);       break;
                case 4: dir.search(2);       break;
                case 5: dir.editContact();   break;
                case 6: dir.deleteContact(); break;
                case 0: cout << "Goodbye!\n"; break;
                default: cout << "Invalid choice. Enter a number from 0 to 6.\n";
            }
        } catch (const exception& ex) {
            cout << "Error: " << ex.what() << "\n";
        }
    } while (choice != 0);

    return 0;
}
