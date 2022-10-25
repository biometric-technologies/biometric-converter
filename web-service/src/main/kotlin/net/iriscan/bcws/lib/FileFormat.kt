package net.iriscan.bcws.lib

/**
 * @author Slava Gornostal
 */
enum class FileFormat {
    WSQ, JPEG, IHEAD, JPEG2000, PNG,
    ISO, ISOC, ISOCC, ANSI;

    fun isMinutae(): Boolean = arrayOf(ISO, ISOC, ISOCC, ANSI).contains(this)
    fun isImage(): Boolean = !isMinutae()


}