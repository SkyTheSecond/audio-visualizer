# Vendored dependencies

Sources copied in-tree so the project builds offline and can be patched locally.

| Library    | Version | Upstream                             | License                     |
|------------|---------|--------------------------------------|-----------------------------|
| googletest | v1.18.0 | https://github.com/google/googletest | BSD-3-Clause                |
| glfw       | 3.4     | https://github.com/glfw/glfw         | Zlib                        |
| imgui      | v1.91.5 | https://github.com/ocornut/imgui     | MIT                         |
| glad       | 2.0.8   | https://github.com/Dav1dde/glad      | see SPDX line in gl.h       |

glad is generated output, not upstream source. Regenerate with:

    python -m glad --api gl:core=4.6 --out-path third_party/glad \
        --extensions GL_KHR_debug --reproducible c
