#include "faceveil/ImageScanner.hpp"

#include <QFileInfo>

#include <algorithm>
#include <system_error>

namespace faceveil
{
    namespace
    {
        std::string lowercaseExtension(const std::filesystem::path &path)
        {
            auto extension = path.extension().string();
            std::ranges::transform(extension, extension.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
            return extension;
        }

        bool escapesBase(const std::filesystem::path &relative)
        {
            for (const auto &part: relative)
            {
                if (part == "..")
                {
                    return true;
                }
            }
            return false;
        }

        void appendFile(std::vector<ScanResult> &results,
                        const std::filesystem::path &file,
                        const std::filesystem::path &base)
        {
            if (!isSupportedImage(file))
            {
                return;
            }

            std::error_code error;
            auto relative = std::filesystem::relative(file, base, error);
            if (error || relative.empty() || relative.is_absolute() || escapesBase(relative))
            {
                relative = file.filename();
            }
            results.push_back({file, relative});
        }
    } // namespace

    bool isSupportedImage(const std::filesystem::path &path)
    {
        const auto extension = lowercaseExtension(path);
        return extension == ".jpg" || extension == ".jpeg" || extension == ".png" ||
               extension == ".bmp" || extension == ".tif" || extension == ".tiff" ||
               extension == ".webp";
    }

    std::vector<ScanResult> scanImages(const QStringList &inputs, bool recursive)
    {
        std::vector<ScanResult> results;

        for (const auto &input: inputs)
        {
            const QFileInfo info(input);
            const auto path = std::filesystem::path(input.toStdString());

            if (info.isFile())
            {
                appendFile(results, path, path.parent_path());
                continue;
            }

            if (!info.isDir())
            {
                continue;
            }

            std::error_code error;
            if (recursive)
            {
                for (const auto &entry: std::filesystem::recursive_directory_iterator(path, error))
                {
                    if (error)
                    {
                        break;
                    }
                    if (entry.is_regular_file())
                    {
                        appendFile(results, entry.path(), path);
                    }
                }
            } else
            {
                for (const auto &entry: std::filesystem::directory_iterator(path, error))
                {
                    if (error)
                    {
                        break;
                    }
                    if (entry.is_regular_file())
                    {
                        appendFile(results, entry.path(), path);
                    }
                }
            }
        }

        std::ranges::sort(results, [](const ScanResult &a, const ScanResult &b)
        {
            return a.sourcePath.string() < b.sourcePath.string();
        });

        results.erase(std::ranges::unique(results, [](const ScanResult &a, const ScanResult &b)
                      {
                          return a.sourcePath == b.sourcePath;
                      }).begin(),
                      results.end());

        return results;
    }
} // namespace faceveil
