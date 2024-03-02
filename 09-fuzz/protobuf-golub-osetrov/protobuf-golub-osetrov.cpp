#include "protobuf-golub-osetrov.hpp"

#include <bitset>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

union float_and_int {
  float f;
  uint32_t i;
};

std::string FloatToString(float f) {
  float_and_int fi{f};
  std::bitset<32> b{fi.i};

  return (std::stringstream{} << b).str();
}

}  // namespace

std::string ErrorToString(Error error) {
  switch (error) {
    case Error::NO_ERROR:
      return "No error";
    case Error::FAILED_TO_SERIALIZE_TO_PROTO:
      return "Failed to serialize to proto";
    case Error::FAILED_TO_PARSE_PROTO:
      return "Failed to parse proto";
  }
}

void buildProtoStudent(format04::Student* res, const Student& student) {
  res->set_name(student.name);
  res->set_login(student.login);
  res->set_group(student.group);

  uint32_t practice = 0;
  for (uint32_t i = 0; i < 8; i++) {
    if (student.practice[i]) {
      practice |= (1u << i);
    }
  }
  res->set_practice(practice);

  res->set_project_repo(student.project.repo);
  res->set_project_mark(student.project.mark);
  res->add_mark(student.mark);
}

Student parseProtoStudent(const format04::Student& student) {
  Student res;
  student.name().copy(res.name, sizeof(res.name));
  student.login().copy(res.login, sizeof(res.login));
  student.group().copy(res.group, sizeof(res.group));
  for (uint32_t i = 0; i < 8; i++) {
    if (student.practice() & (1u << i)) {
      res.practice[i] = 1;
    }
  }
  student.project_repo().copy(res.project.repo, sizeof(res.project.repo) - 1);
  res.project.mark = student.project_mark();
  res.mark = student.mark(0);

  return res;
}

Error SerializeToProtobuf(const std::string& path) {
#ifndef FUZZING_ENABLED
  std::cout << "Reading binary student data from " << path << std::endl;
#endif

  auto input = fopen(path.c_str(), "r");
  Student data;
  std::vector<Student> students;

  while (fread(&data, sizeof(Student), 1, input)) {
    students.push_back(std::move(data));
#ifndef FUZZING_ENABLED
    std::cout << "Student mark: " << std::fixed << std::setprecision(20)
              << FloatToString(students.back().mark) << std::endl;
#endif
  }
  fclose(input);
#ifndef FUZZING_ENABLED
  std::cout << students.size() << " students read..." << std::endl;
#endif

  format04::Students result;
  for (const auto& student : students) {
    auto* protoStudent = result.add_student();
    buildProtoStudent(protoStudent, student);
#ifndef FUZZING_ENABLED
    std::cout << "Student mark proto: " << std::fixed << std::setprecision(20)
              << FloatToString(protoStudent->mark(0)) << std::endl;
#endif
  }

  const std::string output_path = path.substr(0, path.size() - 4) + ".protobuf";

#ifndef FUZZING_ENABLED
  std::cout << "written to " << output_path << "..." << std::endl;
#endif
  std::ofstream output(output_path,
                       std::ios::trunc | std::ios::out | std::ios::binary);
  /// @bug 1. No check for protobuf serialization error, namely incorrect UTF-8
  if (!result.SerializePartialToOstream(&output)) {
    return Error::FAILED_TO_SERIALIZE_TO_PROTO;
  }
  output.close();

  return Error::NO_ERROR;
}

Error SerializeToBin(const std::string& path) {
#ifndef FUZZING_ENABLED
  std::cout << "Reading protobuf student data from " << path << std::endl;
#endif

  std::ifstream input(path, std::ios::binary | std::ios::in);
  format04::Students protoStudents;
  if (!protoStudents.ParseFromIstream(&input)) {
    return Error::FAILED_TO_PARSE_PROTO;
  }
  input.close();

  std::vector<Student> binStudents;
  for (auto& student : protoStudents.student()) {
    binStudents.emplace_back(parseProtoStudent(student));

#ifndef FUZZING_ENABLED
    std::cout << "Student mark: " << std::fixed << std::setprecision(20)
              << FloatToString(binStudents.back().mark) << std::endl;
#endif
  }

#ifndef FUZZING_ENABLED
  std::cout << binStudents.size() << " students read..." << std::endl;
#endif

  const std::string output_path = path.substr(0, path.size() - 9) + ".bin";
#ifndef FUZZING_ENABLED
  std::cout << "written to " << output_path << "..." << std::endl;
#endif
  std::ofstream output(output_path,
                       std::ios::trunc | std::ios::out | std::ios::binary);

  for (const auto student : binStudents) {
    output.write((char*)&student, sizeof(Student));
  }
  output.close();

  return Error::NO_ERROR;
}
