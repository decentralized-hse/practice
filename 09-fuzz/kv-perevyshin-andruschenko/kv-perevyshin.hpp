#include "helpers.hpp"

using namespace std;

void dumpString(ofstream& out, const string& str, const string& field, size_t id) {
    string formattedStr = std::regex_replace(str, std::regex(R"(\n)"), "\\n");
    out << "[" << id << "]." << field << ": " << formattedStr<< "\n";
}

void dumpFloat(ofstream& out, float f, size_t id) {
    array<char, 20> buf;
    to_chars_result result = to_chars(buf.data(), buf.data() + buf.size(), f);
    out << "[" << id << "].mark: " << string_view(buf.data(), result.ptr - buf.data()) << "\n";
}

void dumpStudentKV(ofstream& out, const Student& student, size_t id) {
    dumpString(out, std::string(std::string_view(student.name, 32)), "name", id);  // фикс на отсуствие гарантии null-терминирования
    dumpString(out, std::string(std::string_view(student.login, 16)), "login", id);  // фикс на отсуствие гарантии null-терминирования
    dumpString(out, std::string(std::string_view(student.group, 8)), "group", id);  // фикс на отсуствие гарантии null-терминирования
    dumpString(out, std::string(std::string_view(student.project.repo, 63)), "project.repo", id);  // фикс на отсуствие гарантии null-терминирования
    
    out << "[" << id << "].project.mark: " << static_cast<uint32_t>(student.project.mark) << "\n";
    out << "[" << id << "].mark: " << *reinterpret_cast<const uint32_t*>(&student.mark) << "\n";  // фикс записи флотов в kv-формате (используем uint32_t как промежуточное представление)
    
    for (size_t j = 0; j < Student::practiceSize; ++j) {
        out << "[" << id << "].practice.[" << j << "]: " <<  static_cast<uint32_t>(student.practice[j]) << "\n"; 
    }
}

void dumpFromBinToKv(const string& mainFileName) {
    auto in = fopen((mainFileName + ".bin").c_str(), "rb");  // фикс неправильного режима чтения файла
    cout << "Reading binary student data from " << mainFileName << ".bin..." << endl;

    ofstream out(mainFileName + ".kv");

    Student student; // фикс (09-fuzz) для проверки наличия неполных объектов
    size_t id = 0;
    size_t last_read = 0;
    while ((last_read = fread(&student, sizeof(char), sizeof(Student), in))) {  // фикс (09-fuzz) для проверки наличия неполных объектов
        if (last_read < sizeof(Student)) {  // фикс (09-fuzz) для проверки наличия неполных объектов
            fclose(in);
            throw MalformedInputException();
        }
        if (!is_student_valid(&student)) {  // фикс на отсутствие валидации входных данных
            fclose(in);  // фикс незакрытых файловых дескрипторов
            throw MalformedInputException();
        }
        dumpStudentKV(out, student, id);
        ++id;
    }
    fclose(in);  // фикс незакрытых файловых дескрипторов
    cout << id << " students read..." << endl;
    cout << "written to " << mainFileName << ".kv..." << endl;
}

void loadString(const string& str, char field[]) {
    string formattedStr = std::regex_replace(str, std::regex(R"(\\n)"), "\n");
    memcpy(field, formattedStr.c_str(), formattedStr.size());
}

size_t loadPracticeId(const string& fieldName) {
    static size_t practiceSize = strlen("practice");
    size_t practiceIdSize = fieldName.size() - practiceSize - 3;
    string practiceId = fieldName.substr(practiceSize + 2, practiceIdSize);
    return atoll(practiceId.c_str());
}

bool loadStudentField(Student& student, const string& fieldName, const string& value) {
    if (fieldName == "name") {
        loadString(value, student.name);
    } else if (fieldName == "login") {
        loadString(value, student.login);
    } else if (fieldName == "group") {
        loadString(value, student.group);
    } else if (fieldName == "project.repo") {
        loadString(value, student.project.repo);
    } else if (fieldName == "project.mark") {
        student.project.mark = atoll(value.c_str());
    } else if (fieldName == "mark") {
        const uint32_t student_mark_as_ui32 = atoll(value.c_str());
        student.mark = *reinterpret_cast<const float*>(&student_mark_as_ui32);  // фикс записи nan в kv-формате (используем uint32_t как промежуточное представление)
    } else if (fieldName.rfind("practice") == 0) {
        auto id = loadPracticeId(fieldName);
        student.practice[id] = atoi(value.c_str());
    } else {
        return false;
    }

    return true;
}

tuple<string, string, string> getStudentInfoFromKV(const string& line) {
    size_t delimeter = line.find_first_of(":");
    size_t studentIdDelimeter = line.find_first_of(".");

    string studentId = line.substr(1, studentIdDelimeter - 1);

    size_t keySize = delimeter - studentId.size() - 2;
    string key = line.substr(studentIdDelimeter + 1, keySize);

    string value = line.substr(delimeter + 2);

    return {studentId, key, value};
}

void dumpStudentBin(FILE* out, const Student& student, const string& id) {
    if (!id.empty()) {
        fwrite(&student, sizeof(Student), 1, out);
    }
}

void dumpFromKvToBin(const string& mainFileName) {
    ifstream in(mainFileName + ".kv");
    cout << "Reading kv student data from " << mainFileName << ".kv..." << endl;
    
    auto out = fopen((mainFileName + ".bin").c_str(), "w");

    Student student{};
    string prevStudentId = "";
    string line;
    while (getline(in, line)) {
        auto [studentId, key, value] = getStudentInfoFromKV(line);

        if (studentId != prevStudentId) {
            dumpStudentBin(out, student, prevStudentId);
            prevStudentId = studentId;
            student = Student{};
        }

        if (!loadStudentField(student, key, value)) {
            cout << "Error. Wrong type of structure field. Line: " << line << " " << key << endl;
            fclose(out);
            exit(1);
        }
    }
    dumpStudentBin(out, student, prevStudentId);

    if (prevStudentId.empty()) {
        prevStudentId = "-1";
    }

    fclose(out);

    cout << atoi(prevStudentId.c_str()) + 1 << " students read..." << endl;
    cout << "written to " << mainFileName << ".bin..." << endl;
}

bool chooseFormat(const string& path) {
    size_t last_dot = path.find_last_of(".");
    string fileName = path.substr(0, last_dot);
    string fileType = path.substr(last_dot + 1);

    if (fileType == "bin") {
        dumpFromBinToKv(fileName);
    } else if (fileType == "kv") {
        dumpFromKvToBin(fileName);
    } else {
        return false;
    }
    
    return true;
}

int execute_main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Error. Add path of file with a bin or kv dump in the command parameters" << endl;
        return 1;
    }

    try {
        if (!chooseFormat(argv[1])) {
            cout << "Error. Unsupported file type. Choose file with .bin or .kv" << endl;
            return 1;
        }
    } catch (MalformedInputException exc) {
        cerr << exc.what() << "\n";
        return -1;
    }

    return 0;
}
