#include "texture.hpp"

#include <glad/gl.h>

#include <cstddef>
#include <format>
#include <utility>

namespace gui {
Texture::~Texture() {
    if (id_ != 0) glDeleteTextures(1, &id_);
}

Texture::Texture(Texture&& other) noexcept
    : id_(std::exchange(other.id_, 0)),
      width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0)) {}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (id_ != 0) glDeleteTextures(1, &id_);  // release what we currently own
        id_ = std::exchange(other.id_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
    }
    return *this;
}

std::expected<Texture, std::string> Texture::create(int width, int height,
                                                    std::span<const unsigned char> rgba) {
    if (width <= 0 || height <= 0) {
        return std::unexpected("texture size must be positive");
    }

    const std::size_t expected_bytes =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    if (rgba.size() != expected_bytes) {
        return std::unexpected(
            std::format("pixel buffer holds {} bytes, expected {}", rgba.size(), expected_bytes));
    }

    GLint max_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
    if (width > max_size || height > max_size) {
        return std::unexpected(
            std::format("{} x {} exceeds this GPU's maximum texture size of {}. "
                        "Use a shorter or lower-sample-rate file.",
                        width, height, max_size));
    }

    Texture texture;

    if (GLAD_GL_VERSION_4_5) {
        // Direct State Access: every call names the texture it acts on.
        glCreateTextures(GL_TEXTURE_2D, 1, &texture.id_);

        // Immutable storage: one mip level, 8 bits per channel RGBA.
        glTextureStorage2D(texture.id_, 1, GL_RGBA8, width, height);

        glTextureParameteri(texture.id_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture.id_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(texture.id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture.id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTextureSubImage2D(texture.id_, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                            rgba.data());
    } else {
        // Pre-4.5 drivers (macOS stops at 4.1): classic bind-to-edit. Put the previous
        // binding back afterwards so whoever had it bound isn't surprised.
        GLint previous = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);

        glGenTextures(1, &texture.id_);
        glBindTexture(GL_TEXTURE_2D, texture.id_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     rgba.data());

        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous));
    }

    texture.width_ = width;
    texture.height_ = height;
    return texture;
}
}  // namespace gui
