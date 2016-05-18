#ifndef __ERROR_MESSAGE__
#define __ERROR_MESSAGE__

#include <string>
#include <sstream>

class ErrorMessage {
public:
  ErrorMessage(std::string location, std::string message) : Location(location), Message(message) {}

  ErrorMessage(std::string location, std::ostringstream &stream) : Location(location), Message(stream.str()) {}

  std::string &getLocation() {
    return Location;
  }
  std::string &getMessage() {
    return Message;
  }

private:
  std::string Location;
  std::string Message;
};

#endif
