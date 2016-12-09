struct compareSig : public std::binary_function<event_sgnt, event_sgnt, bool> {
  bool operator()(const event_sgnt &lhs, const event_sgnt &rhs) const {
    return (lhs.type < rhs.type) ||
           (lhs.type == rhs.type && lhs.code < rhs.code) ||
           (lhs.type == rhs.type && lhs.code == rhs.code &&
            lhs.value < rhs.value);
  }
};
