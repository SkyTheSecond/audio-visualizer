#pragma once

#include <expected>
#include <span>
#include <string>

namespace gui {

// Owns one OpenGL 2D texture. Move-only.
// Both creating and destroying one need a current OpenGL context.
class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Upload RGBA pixels (4 bytes each, rows top to bottom) as a new texture.
    // Returns an error, not a black rectangle, if the size is wrong or too big for the GPU.
    static std::expected<Texture, std::string> create(int width, int height,
                                                      std::span<const unsigned char> rgba);

    // The OpenGL texture name, for handing to ImGui. Plain unsigned int (what GLuint is)
    // so this header doesn't need glad.
    unsigned int id() const { return id_; }
    int width() const { return width_; }
    int height() const { return height_; }

    explicit operator bool() const { return id_ != 0; }

private:
    unsigned int id_ = 0;
    int width_ = 0;
    int height_ = 0;
};
}  // namespace gui
