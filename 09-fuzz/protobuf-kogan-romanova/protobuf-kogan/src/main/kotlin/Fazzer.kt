import com.code_intelligence.jazzer.api.FuzzedDataProvider
import com.code_intelligence.jazzer.api.FuzzerSecurityIssueMedium
import com.google.protobuf.ByteString
import protobuf.kogan.*
import protobuf.kogan.Struct.Student
import java.nio.file.StandardOpenOption
import java.nio.file.StandardOpenOption.*
import kotlin.io.path.*
import createProtobufStruct
import convertProtobufToByteArray

object KotlinFuzzer {

    @JvmStatic
    fun fuzzerTestOneInput(data: FuzzedDataProvider) {
        val test_data: ByteArray = ByteArray(0)
        for (i in 0..127) {
            test_data.plus(data.consumeByte())
        }
        checkBinConvert(test_data)

        val studentsList = listOf<Struct.Student>()
        for (i in 0..3) {
            val name = data.consumeString(32)
            val login = data.consumeString(16)
            val group = data.consumeString(8)
            val practice: ByteString = ByteString.copyFrom(data.consumeBytes(8))
            val repo = data.consumeString(59)
            val markInner: ByteString = ByteString.copyFrom(data.consumeBytes(1))
            val markOuter: Float = data.consumeFloat()
            val curStudent: Struct.Student = student {
                this.name = name
                this.login = login
                this.group = group
                this.practice = practice
                this.project = project {
                    this.repo = repo
                    this.mark = markInner
                }
                this.mark = markOuter
            }
            studentsList.plusElement(curStudent)
        }
        val studentsStruct: Struct.Students = Struct.Students.newBuilder().addAllStudents(studentsList).build()
        checkProtoConvert(studentsStruct)
    }

    private fun checkBinConvert(data: ByteArray) {
        if (data.size % 128 != 0) {
            println("Malformed input.")
            return
        }

        val students: Struct.Students = createProtobufStruct(data)

        val destination = "fuzzing_student.protobuf"
        Path(destination).outputStream(CREATE, WRITE, TRUNCATE_EXISTING).use {
            students.writeTo(it)
        }

        val revertData: ByteArray = convertProtobufToByteArray(students)

        assert(data.contentEquals(revertData)) {
            "Assertion failed: ByteArrays are not equal. Expected: ${data.contentToString()}, Actual: ${revertData.contentToString()}"
        }
    }

    private fun checkProtoConvert(studentsStruct: Struct.Students) {
        val data: ByteArray = convertProtobufToByteArray(studentsStruct)

        val destination: String = "fuzzing_student.bin"
        Path(destination).writeBytes(data, CREATE, WRITE, TRUNCATE_EXISTING)

        val revertStudentsStruct: Struct.Students = createProtobufStruct(data)

        assert(studentsStruct == revertStudentsStruct) {
            "Assertion failed: StudentsStructs are not equal. Expected: ${studentsStruct.toString()}, Actual: ${revertStudentsStruct.toString()}"
        }
    }
}