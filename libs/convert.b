/**
 * Convert
 *
 * Bird's data conversion library
 * @copyright 2021, Ore Richard Muyiwa
 */
class Convert {
  
  /**
   * hex_to_bytes(str)
   * converts hexadecimal string of any length to bytes
   */
  static hex_to_bytes(str) {
    var length = str.length()
    var b = bytes(length / 2)

    iter var i = 0; i < length - 1; i += 2 {
      b[i / 2] = to_number('0x${str[i,i+2]}')
    }

    return b
  }
}