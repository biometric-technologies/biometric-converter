package net.iriscan.bcws.lib

import com.sun.jna.Native
import com.sun.jna.NativeLibrary

/**
 * @author Slava Gornostal
 */
object ConverterFactory {
    init {
        NativeLibrary.addSearchPath("converter", "/usr/local/lib")
    }

    val instance: Converter = Native.load("converter", Converter::class.java)
}
