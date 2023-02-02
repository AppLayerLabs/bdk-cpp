#include "hex.h"
#include "utils.h"

Hex::Hex() : _hex(), strict() {}

Hex::Hex(std::string&& value, bool strict) : _hex(std::move(value)), strict(strict)
{
  isHexValid();
  if (!strict)
  {
    if (_hex[0] == '0' && (_hex[1] == 'x' || _hex[1] == 'X')) _hex.erase(0,2);
  } else {
    if (_hex[0] != '0' && (_hex[1] != 'x' || _hex[1] != 'X')) _hex.insert(0, "0x");
  }
  for (auto &c : _hex) if (std::isupper(c)) c = std::tolower(c);
}

Hex::Hex(const std::string_view& value, bool strict) : strict(strict)
{
  isHexValid(value);
  std::string ret;
  uint64_t index = 0;
  if (strict && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) index = 2;
  for (;index < value.size(); ++index) {
    if (std::isupper(value[index])) {
      ret += std::tolower(value[index]);
    } else {
      ret += value[index];
    }
  }
  _hex = ret;
}

const std::string& Hex::get() const { return _hex; }

std::string Hex::bytes() const
{
  std::string ret;
  uint32_t index = _hex.size() % 2 != 0 ? 1 : 0 ;

  // Parse two by two chars until the end
  while (index < _hex.size()) {
    int h = Utils::hexCharToInt(_hex[index]);
    int l = Utils::hexCharToInt(_hex[index+1]);
    if (h != -1 && l != -1) {
      ret += (char) uint8_t(h * 16 + l);
    } else {
      throw std::runtime_error(
              std::string(__func__) + ": " +
              std::string("One or more invalid hex chars: ") +
              _hex[index] + _hex[index + 1]
      );
    }
    index += 2;
  }
  return ret;
}

bool Hex::isHexValid(const std::string_view& v)
{
  std::string hex = v.empty() ? _hex : v.data();
  if (strict && hex.substr(0, 2) != "0x" && hex.substr(0, 2) != "0X") {
    throw std::runtime_error(
            std::string("Error at --> Hex::isHexValid():\r\n")
            + "Strict mode requires prefix \"0x\" in \r\n\""
            + hex + "\""
    );
  }

  auto npos = hex.find_first_not_of(filter);
  if (npos != std::string::npos)
  {
    throw std::runtime_error(
            std::string("Error at --> Hex::isHexValid():\r\n")
            + "Invalid character \"" + hex.at(npos) + "\", at position: " + std::to_string(npos) + "\r\n"
            + "Ref (Hex::_hex: \"" + hex + "\")."
    );
  }

  return true;
}

Hex Hex::fromBytes(std::string_view bytes, bool strict) {
  auto _it = bytes.begin();
  auto _end = bytes.end();
 	static char const* hexdigits = "0123456789abcdef";
  size_t off = 0;
 	std::string hex(std::distance(_it, _end)*2, '0');
 	for (; _it != _end; _it++)
 	{
 		hex[off++] = hexdigits[(*_it >> 4) & 0x0f];
 		hex[off++] = hexdigits[*_it & 0x0f];
 	}
  return Hex(std::move(hex), strict);
}

Hex Hex::fromUTF8(std::string_view bytes, bool strict) {
  std::stringstream ss;
  for (int i = 0; i < bytes.length(); i++) {
    // You need two casts in order to properly cast char to uint.
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint>(static_cast<uint8_t>(bytes[i]));
  }

  return Hex(std::move(ss.str()), strict);
}

Hex& Hex::operator+=(const std::string& hex)
{
  if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X'))
  {
    _hex += hex.substr(2);
  }
  else
  {
    _hex += hex;
  }
  return *this;
}

Hex& Hex::operator+=(const Hex& hex)
{
  if (hex._hex[0] == '0' && (hex._hex[1] == 'x' || hex._hex[1] == 'X'))
  {
    _hex += hex._hex.substr(2);
  }
  else
  {
    _hex += hex._hex;
  }
  return *this;
}

std::ostream &operator<<(std::ostream &out, const Hex &hex)  {
  return out << hex._hex;
}

/**
 * TODO: Convert to properly sized bytes
 **/
uint256_t Hex::getUint() const {
  return boost::lexical_cast<HexTo<uint256_t>>(_hex);
}

Hex::~Hex() = default;