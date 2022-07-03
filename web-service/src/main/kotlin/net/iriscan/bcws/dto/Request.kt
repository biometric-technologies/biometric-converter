package net.iriscan.bcws.dto

import net.iriscan.bcws.lib.ConvertOutputType

/**
 * @author Slava Gornostal
 */
data class Request(val input: String, val outputType: ConvertOutputType)