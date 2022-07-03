package net.iriscan.bcws.lib

import com.sun.jna.Library
import com.sun.jna.Native
import com.sun.jna.NativeLibrary
import com.sun.jna.ptr.IntByReference
import com.sun.jna.ptr.PointerByReference

/**
 * @author Slava Gornostal
 */
interface Converter : Library {
    companion object {
        init {
            NativeLibrary.addSearchPath("converter", "/usr/local/lib")
        }

        val instance: Converter = Native.load("converter", Converter::class.java)
    }

    fun convert(
        input: ByteArray,
        inputLength: Int,
        outputType: String,
        output: PointerByReference,
        outputLength: IntByReference
    ): Int
}

enum class ConvertOutputType { ISO, ANSI }