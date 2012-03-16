// The structure of compressed code entries
struct IrCode {
  uint8_t timer_val;
  uint8_t numpairs;
  uint8_t bitcompression;
  uint16_t const *times;
  uint8_t codes[];
};

extern const struct IrCode* const NApowerCodes[];
extern const struct IrCode* const EUpowerCodes[];

const uint8_t num_NAcodes;
const uint8_t num_EUcodes;
