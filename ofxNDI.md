# ofxNDI — Minimal NDI Addon for OpenFrameworks

Personal fork. Stripped down to essentials. Video only, no audio, no metadata.

---

## Architecture

Two classes, no wrapper-on-wrapper:

```
ofxNDIReceiver   — discovers NDI sources, receives video, owns internal ofTexture
ofxNDISender     — sends video from OF types (texture, fbo, image, pixels)
```

Both talk to NDI directly via `Processing.NDI.Lib.h`. No `ofxNDIreceive` / `ofxNDIreceiver` split.

---

## ofxNDIReceiver

### Usage
```cpp
ofxNDIReceiver receiver;

void setup() {
    receiver.setup(); // or setup("My NDI Source") to auto-connect
}

void update() {
    receiver.update(); // discovers + receives
}

void draw() {
    receiver.draw(0, 0); // or draw(x, y, w, h)
}
```

### API
| Method | Description |
|--------|-------------|
| `setup(name)` | Initialize NDI, create finder, auto-connect if name matches |
| `close()` | Release everything |
| `update()` | Refresh sender list, receive video frame, resize texture if needed |
| `draw(x, y)` / `draw(x, y, w, h)` | Draw internal texture |
| `getTexture()` | Access internal `ofTexture&` |
| `isConnected()` | True after first video frame received |
| `isInitialized()` | True after successful setup |
| `getWidth()` / `getHeight()` | Texture dimensions |
| `getSenderCount()` | Number of sources on network |
| `getSenderName(i)` | Source name at index |
| `connect(name)` / `connect(index)` | Switch to different source |
| `getCurrentSenderName()` | Currently connected source name |

### Sender Discovery
`update()` calls `refreshSenders()` every frame. This polls `find_get_current_sources` and rebuilds the list when **count changes OR any name changes at the same count**. This handles:
- Adding a source → detected
- Removing a source → detected  
- A quits, B starts simultaneously → detected

### Color Format Handling
Receiver requests `NDIlib_recv_color_format_BGRA_BGRA` from NDI. The SDK should deliver BGRA/BGRX which we swizzle to RGBA. If UYVY or other formats arrive (rare with this setting), we copy raw and log a warning — colors will be wrong but it won't crash.

### Texture Resize
`pixelBuffer.allocate()` + `texture.allocate()` only when `videoFrame.xres/yres` differ from current. No shader, no FBO, no PBO.

---

## ofxNDISender

### Usage
```cpp
ofxNDISender sender;

void setup() {
    sender.setup("My NDI Output");
}

void draw() {
    sender.send(myTexture);
}
```

### API
| Method | Description |
|--------|-------------|
| `setup(name)` | Create NDI sender |
| `close()` | Release sender |
| `send(texture/fbo/image/pixels)` | Send frame |
| `send(pixels, w, h)` | Send raw RGBA buffer |
| `isInitialized()` | — |
| `getName()` | Sender name |

### TODO / Known Issues
- [ ] `sendFrame()` hardcodes 59.94 fps (`60000/1001`). Should match app framerate or be configurable.
- [ ] No YUV/RGBA format choice — always sends RGBA.
- [ ] No async send option (`send_send_video_async_v2`).
- [ ] No sender example.

---

## Shaders Inline?

The old addon used external shader files in `bin/data/yuv2rgba/` and `bin/data/rgba2yuv/`. These could be inlined as `const char*` strings in the source to eliminate file dependencies:

```cpp
static const char* yuv2rgbaVert = R"(
    #version 150
    in vec2 position;
    void main() { gl_Position = vec4(position, 0.0, 1.0); }
)";
```

**Pros:** No data folder, no file I/O, no path headaches, self-contained.
**Cons:** Harder to edit, platform-specific GL version strings needed (GL2/GL3/GLES).

For the receiver, if we need UYVY support, inline shaders are the way to go. For now, requesting BGRA from NDI avoids the need.

---

## Build

macOS (chalet):
```yaml
links:
  - ndi
linkerOptions[:macos]:
  - -rpath @executable_path
```

NDI runtime must be installed (`/usr/local/lib/libndi.dylib`).

---

## Examples

- `example-receiver` — basic receiver, draw + sender list on keypress
- `example-receiver-listener` — detects sender list changes, displays all sources

---

## Changes from upstream

- Removed: `ofxNDIreceive`, `ofxNDIreceiver` (old), `ofxNDIsend`, `ofxNDIsender` (old)
- Removed: `ofxNDIutils`, `ofxNDIdynloader`, `ofxNDIplatforms`, `sse2neon.h`
- Removed: all shader data files (yuv2rgba, rgba2yuv)
- Removed: audio examples, webcam example, Windows examples
- Removed: PBO async upload, YUV shader path, fps counter, metadata, audio
- Kept: `libs/NDI/include/` headers in same location
