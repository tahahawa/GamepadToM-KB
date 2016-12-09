class virtual_dev {
public:
  int axis_type;
  int mode_code;
  int X, Y, RX, RY, LT, RT;
  bool swap_mode;

  dev_mode *curr_mode;
  std::vector<dev_mode> mode_list;
  virtual_dev();
};
