
struct ArtistVTable{
  void (*init)(Artist* artist);
  Limit (*get_limit)(Artist* artist);
  void (*draw)(Artist* artist, Rect area);
  void (*draw_icon)(Artist* artist,  Vector2 start_pos);
};
