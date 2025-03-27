import java.io.BufferedReader
import java.io.File
import java.io.FileReader
import java.io.IOException
import kotlin.system.exitProcess

fun searchDirectory(path: String, root: String): String {
    val directory = File(root)
    if (directory.isDirectory) {
        return path
    }
    if (path == "." || path == "./" || path.isEmpty()) {
        return root
    }
    val tokens = path.split("/")
    if (tokens.size < 2) {
        System.err.println("Директория не найдена:\n\tпуть: $path\n\tкорень: $root\n")
        throw RuntimeException("Некорректный ввод")
    }
    val pathPrefix = "${tokens[0]}/"
    val pathSuffix = tokens[1]
    BufferedReader(FileReader(root)).use { file ->
        var line: String?
        while (file.readLine().also { line = it } != null) {
            val inputLines = line!!.split("\t")
            if (pathPrefix == inputLines[0]) {
                return searchDirectory(pathSuffix, inputLines[1])
            }
        }
    }
    System.err.println("Префикс не найден:\n\tпрефикс: $pathPrefix\n\tсуффикс: $pathSuffix\n")
    throw RuntimeException("Некорректный ввод")
}

fun outputDirectory(hash: String, prefix: String, isRoot: Boolean) {
    BufferedReader(FileReader(hash)).use { file ->
        var line: String?
        while (file.readLine().also { line = it } != null) {
            val inputLines = line!!.split("\t")
            if (inputLines.size < 2) {
                continue
            }
            val objectName = inputLines[0]
            val objectHash = inputLines[1]
            if (objectName.endsWith(":")) {
                println(prefix + objectName.substring(0, objectName.length - 1))
            } else {
                println(prefix + objectName)
                if (objectName.substring(0, objectName.length - 1) == ".parent" && isRoot) {
                    continue
                }
                outputDirectory(objectHash, prefix + objectName, false)
            }
        }
    }
}

fun main(args: Array<String>) {
    if (args.size != 2) {
        System.err.println("Должно быть передано 2 аргумента")
        exitProcess(1)
    }
    val path = args[0]
    val root = args[1]
    try {
        val hash = searchDirectory(path, root)
        outputDirectory(hash, "", hash == root)
    } catch (e: IOException) {
        System.err.println("Ошибка. Проверьте корректность переданных аргументов\n" + e.message)
        exitProcess(1)
    }
}
