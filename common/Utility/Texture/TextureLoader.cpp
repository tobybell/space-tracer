#include "common/Utility/Texture/TextureLoader.h"
#include "common/Rendering/Textures/Texture2D.h"
#include "FreeImage.h"
#include <bitset>

namespace TextureLoader
{

unsigned char* LoadRawData(const std::string& filename, int& width, int& height)
{
#ifndef ASSET_PATH
    static_assert(false, "ASSET_PATH is not defined. Check to make sure your projects are setup correctly");
#endif
    const std::string completeFilename = std::string(STRINGIFY(ASSET_PATH)) + "/" + filename;
    // Determine File type
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(completeFilename.c_str());
    if (fif == FIF_UNKNOWN) {
        fif = FreeImage_GetFIFFromFilename(completeFilename.c_str());
    }

    if (fif == FIF_UNKNOWN) {
        std::cerr << "ERROR: Failed to determine the filetype for " << filename << std::endl;
        return nullptr;
    }

    // Load image and make sure it's an RGBA texture.
    // This requirement is completely arbitrary.
    FIBITMAP* inputImage = FreeImage_Load(fif, completeFilename.c_str(), 0);
    if (!inputImage) {
        std::cerr << "ERROR: Failed to read in the texture from - " << filename << std::endl;
        return nullptr;
    }

    // Create OpenGL texture and store it in our wrapper.
    width = FreeImage_GetWidth(inputImage);
    height = FreeImage_GetHeight(inputImage);

    unsigned char* textureRawData = new unsigned char[width * height * 4];

    RGBQUAD rgb;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            FreeImage_GetPixelColor(inputImage, x, y, &rgb);
            int idx = x * 4 + y * width * 4;
            textureRawData[idx] = (unsigned char)rgb.rgbRed;
            textureRawData[idx + 1] = (unsigned char)rgb.rgbGreen;
            textureRawData[idx + 2] = (unsigned char)rgb.rgbBlue;
            textureRawData[idx + 3] = 255;
        }
    }

    FreeImage_Unload(inputImage);

    return textureRawData;
}

// The FreeImage PDF documentation is useful to parse what's going on here.
// Link to the download: http://sourceforge.net/projects/freeimage/files/Source%20Documentation/3.17.0/FreeImage3170.pdf/download?use_mirror=iweb
// The PDF is also included in the external/freeimage folder.
// This function is based off of: http://r3dux.org/2014/10/how-to-load-an-opengl-texture-using-the-freeimage-library-or-freeimageplus-technically/
std::shared_ptr<Texture2D> LoadTexture(const std::string& filename)
{
    int width, height;
    unsigned char* textureRawData = LoadRawData(filename, width, height);
    std::shared_ptr<Texture2D> newTexture = std::make_shared<Texture2D>(textureRawData, width, height);
    return newTexture;
}

}
