package net.iriscan.bcws.lib

/**
 * @author Slava Gornostal
 */
enum class FileFormat(private val libraryType: String? = null) {
    WSQ, JPEG, IHEAD, JPEG2000, PNG,
    ANSI("ANSI"), ISO("ISO"), ISONC("ISONC"), ISOCC("ISOCC"),

    /** Former name of [ISONC], still accepted. */
    ISOC("ISONC");

    fun isMinutae(): Boolean = libraryType != null
    fun isImage(): Boolean = !isMinutae()

    /** The type string libconverter expects for this minutiae format. */
    fun libraryName(): String = libraryType ?: throw IllegalStateException("$name is not a minutiae format")
}
