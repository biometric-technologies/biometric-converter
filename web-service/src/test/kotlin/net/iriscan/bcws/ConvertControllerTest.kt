package net.iriscan.bcws

import com.sun.jna.NativeLibrary
import com.sun.jna.ptr.IntByReference
import com.sun.jna.ptr.PointerByReference
import net.iriscan.bcws.dto.BatchRequest
import net.iriscan.bcws.dto.BatchRequestList
import net.iriscan.bcws.dto.BatchResponseList
import net.iriscan.bcws.dto.Request
import net.iriscan.bcws.dto.Response
import net.iriscan.bcws.lib.ConverterFactory
import net.iriscan.bcws.lib.FileFormat
import org.junit.jupiter.api.Assertions.assertArrayEquals
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertFalse
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import org.junit.jupiter.api.condition.EnabledIf
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.test.web.reactive.server.WebTestClient
import org.springframework.test.web.reactive.server.expectBody
import java.io.File
import java.util.Base64

/**
 * End-to-end tests of the REST API against the native libconverter.
 *
 * They need libconverter (and the BiomDI libfmr it uses) installed where JNA
 * finds it, as on a machine set up for the service. Without it they are
 * skipped, unless CONVERTER_TESTS_REQUIRED is set (as in CI), where they fail.
 */
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@EnabledIf("nativeLibraryAvailable")
class ConvertControllerTest {

    companion object {
        @JvmStatic
        fun nativeLibraryAvailable(): Boolean {
            if (System.getenv("CONVERTER_TESTS_REQUIRED") != null)
                return true
            return try {
                NativeLibrary.addSearchPath("converter", "/usr/local/lib")
                NativeLibrary.getInstance("converter")
                true
            } catch (e: UnsatisfiedLinkError) {
                false
            }
        }

        // The sample image of the repository, a NIST WSQ fingerprint at 500 ppi
        private val sample: ByteArray = File("../example/sample_image.wsq").readBytes()
        private val sampleBase64: String = Base64.getEncoder().encodeToString(sample)
    }

    @Autowired
    lateinit var client: WebTestClient

    private fun convert(input: String, from: FileFormat, to: FileFormat): ByteArray {
        val response = client.post().uri("/convert")
            .bodyValue(Request(input, from, to))
            .exchange()
            .expectStatus().isOk
            .expectBody<Response>()
            .returnResult().responseBody!!
        return Base64.getDecoder().decode(response.output)
    }

    // What the library itself returns, to compare the service with
    private fun libraryImg2fmr(type: String): ByteArray {
        val out = PointerByReference()
        val len = IntByReference()
        val lib = ConverterFactory.instance
        assertEquals(0, lib.img2fmr(sample, sample.size, type, out, len), lib.converter_last_error())
        try {
            return out.value.getByteArray(0, len.value)
        } finally {
            lib.converter_free(out.value)
        }
    }

    @Test
    fun `image conversion returns the requested format`() {
        // Earlier versions passed the input type to img2fmr and always
        // returned ANSI, whatever outputType asked for.
        for (type in listOf("ANSI", "ISO", "ISONC", "ISOCC")) {
            val output = convert(sampleBase64, FileFormat.WSQ, FileFormat.valueOf(type))
            assertArrayEquals(libraryImg2fmr(type), output, "WSQ to $type")
        }
        val iso = convert(sampleBase64, FileFormat.WSQ, FileFormat.ISO)
        val ansi = convert(sampleBase64, FileFormat.WSQ, FileFormat.ANSI)
        assertFalse(iso.contentEquals(ansi), "ISO and ANSI output are the same bytes")
        // Both formats start with "FMR\0"
        assertEquals("FMR", String(iso, 0, 3, Charsets.US_ASCII))
    }

    @Test
    fun `minutiae records convert in both directions`() {
        val iso = convert(sampleBase64, FileFormat.WSQ, FileFormat.ISO)
        val ansi = convert(Base64.getEncoder().encodeToString(iso), FileFormat.ISO, FileFormat.ANSI)
        assertEquals("FMR", String(ansi, 0, 3, Charsets.US_ASCII))
        val back = convert(Base64.getEncoder().encodeToString(ansi), FileFormat.ANSI, FileFormat.ISO)
        assertEquals(iso.size, back.size, "ISO -> ANSI -> ISO changes the record length")
    }

    @Test
    fun `ISOC is still accepted as the name of the normal card format`() {
        val card = convert(sampleBase64, FileFormat.WSQ, FileFormat.ISONC)
        val cardBase64 = Base64.getEncoder().encodeToString(card)
        assertArrayEquals(
            convert(cardBase64, FileFormat.ISONC, FileFormat.ISO),
            convert(cardBase64, FileFormat.ISOC, FileFormat.ISO)
        )
    }

    @Test
    fun `an unsupported combination is a bad request`() {
        client.post().uri("/convert")
            .bodyValue(Request(sampleBase64, FileFormat.ANSI, FileFormat.WSQ))
            .exchange()
            .expectStatus().isBadRequest
            .expectBody().jsonPath("$.message").value<String> { assertTrue(it.contains("not supported"), it) }
    }

    @Test
    fun `a failed conversion is a bad request with the library's reason`() {
        val garbage = Base64.getEncoder().encodeToString("not an image".toByteArray())
        client.post().uri("/convert")
            .bodyValue(Request(garbage, FileFormat.WSQ, FileFormat.ISO))
            .exchange()
            .expectStatus().isBadRequest
            .expectBody().jsonPath("$.message").value<String> {
                assertTrue(it.contains("unrecognized image format"), it)
            }
    }

    @Test
    fun `a batch runs its items in parallel with the same results`() {
        val single = convert(sampleBase64, FileFormat.WSQ, FileFormat.ISO)
        val items = (1..8).map { BatchRequest("finger-$it", sampleBase64, FileFormat.WSQ, FileFormat.ISO) }
        val response = client.post().uri("/convert-batch")
            .bodyValue(BatchRequestList(items))
            .exchange()
            .expectStatus().isOk
            .expectBody<BatchResponseList>()
            .returnResult().responseBody!!
        assertEquals(items.map { it.id }, response.data.map { it.id })
        for (item in response.data)
            assertArrayEquals(single, Base64.getDecoder().decode(item.output), item.id)
    }
}
