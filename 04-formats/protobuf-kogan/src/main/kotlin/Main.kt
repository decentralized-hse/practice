import com.google.protobuf.ByteString
import com.google.protobuf.kotlin.get
import protobuf.kogan.*
import protobuf.kogan.Struct.Student
import java.io.File
import java.io.Writer
import java.nio.ByteBuffer
import java.nio.file.StandardOpenOption
import java.nio.file.StandardOpenOption.*
import kotlin.io.path.*

val MESSAGE_LENGTH = 128

fun createProtobufStruct(data: ByteArray): Struct.Students = students {
    val byteBuffer: ByteBuffer = ByteBuffer.wrap(data)
    val byteString: ByteString = ByteString.copyFrom(data)
    for (i in data.indices step MESSAGE_LENGTH) {
        val name = String(data, i, 32)
        val login = String(data, i + 32, 16)
        val group = String(data, i + 48, 8)
        val practice: ByteString = byteString.substring(i + 56, i + 64)
        val repo = String(data, i + 64, 59)
        val markInner: ByteString = byteString.substring(i + 123, i + 124)
        val markOuter: Float = byteBuffer.getFloat(i + 124)
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
        this.students += curStudent
    }
}

fun readBin(source: String) {
    println("Reading ${source}.")
    val sourceFile = File(source)
    if (!sourceFile.exists()) {
        println("Please enter an existing file.")
    }
    val data: ByteArray = sourceFile.readBytes()
    println("Creating Protobuf Structure.")
    val students: Struct.Students = createProtobufStruct(data)
    val destination = "${source.split(".").dropLast(1).joinToString(".")}.protobuf"
    println("Writing Protobuf Structure to ${destination}.")
    val path = Path(destination)
    path.outputStream(CREATE, WRITE, TRUNCATE_EXISTING).use {
        students.writeTo(it)
    }
    println("Success!")
}

fun convertProtobufToByteArray(studentsStruct: Struct.Students): ByteArray {
    var res = ByteArray(0)
    for (i in 0 until studentsStruct.studentsCount) {
        val curStudent = studentsStruct.getStudents(i)
        res += curStudent.name.toByteArray()
        for (i in curStudent.name.toByteArray().size until 32) {
            res += 0.toByte()
        }
        res += curStudent.login.toByteArray()
        for (i in curStudent.login.toByteArray().size until 16) {
            res += 0.toByte()
        }
        res += curStudent.group.toByteArray()
        for (i in curStudent.group.toByteArray().size until 8) {
            res += 0.toByte()
        }
        res += curStudent.practice.toByteArray()
        res += curStudent.project.repo.toByteArray()
        for (i in curStudent.project.repo.toByteArray().size until 59) {
            res += 0.toByte()
        }
        res += curStudent.project.mark.toByteArray()
        var markOuterByteArray = ByteArray(4)
        ByteBuffer.wrap(markOuterByteArray).putFloat(curStudent.mark)
        res += markOuterByteArray
    }
    return res
}

fun writeBin(source: String) {
    println("Reading $source.")
    val sourcePath = Path(source)
    if (!sourcePath.exists()) {
        println("Please enter an existing file.")
    }
    val studentsStruct: Struct.Students = sourcePath.inputStream().use {
        Struct.Students.newBuilder().mergeFrom(it).build()
    }
    println("Gathering Data.")
    val data: ByteArray = convertProtobufToByteArray(studentsStruct)
    val destination: String = "${source.split(".").dropLast(1).joinToString(".")}.bin"
    println("Writing Data to ${destination}.")
    Path(destination).writeBytes(data, CREATE, WRITE, TRUNCATE_EXISTING)
    println("Success!")
}

fun main(args: Array<String>) {
    if (args.size != 1) {
        println("Please pass 1 argument - the path to the file.")
    }
    val path: String = args[0]
    when (path.split(".").last()) {
        "bin" -> readBin(path)
        "protobuf" -> writeBin(path)
        else -> {
            println("Please pass a .bin or a .protobuf file.")
            return
        }
    }
}
