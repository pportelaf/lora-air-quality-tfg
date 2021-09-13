function Decode(port, bytes) {
  if (port == 1) {
    return {
      CO2: (bytes[0] << 8) | (bytes[1]),
      NH3: Number((((bytes[2] << 7) | (bytes[3] >> 1)) * 0.1).toFixed(1)),
      deviceType: "air-quality"
    };
  } else if (port == 2) {
    return {
      error: (bytes[0] >> 6) * -1 - 1,
      deviceType: "air-quality-error"
    }
  }
}