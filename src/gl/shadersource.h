#ifndef PKOMMON_GL_SHADERSOURCE_HPP
#define PKOMMON_GL_SHADERSOURCE_HPP


#include <string>
#include <vector>
#include <initializer_list>

namespace gl
{

class ShaderSource
{
public: // static functions
    /*!
     * \brief Set directory containg shaders.
     * \return **false** on error.
     */
    static bool setRootDir(const std::string& root);

    /*!
     * \brief Get root directory.
     * \return Root directory.
     */
    static std::string getRootDir();

public: // member functions
    /*!
     * \brief Constructor.
     * \param filename Shader file.
     */
    explicit ShaderSource(const std::string& filename);

    /*!
     * \brief Constructor.
     * \param filename Shader file.
     * \param defines Additional defines.
     */
    ShaderSource(const std::string& filename,
            const std::string& defines);

    /*!
     * \brief Constructor.
     * \param filename Shader file.
     * \param defines Additional defines.
     */
    ShaderSource(const std::string& filename,
            std::initializer_list<std::string> defines);

    /*!
     * \brief Copy constructor.
     * \param other Other ShaderSource object.
     */
    ShaderSource(const ShaderSource& other);

    /*!
     * \brief Move constructor.
     * \param other Other ShaderSource object.
     */
    ShaderSource(ShaderSource&& other) noexcept;

    /*!
     * \brief Copy assignment.
     * \param other Other ShaderSource object.
     * \return Reference to *this;
     */
    ShaderSource& operator=(const ShaderSource& other) &;

    /*!
     * \brief Move assignment.
     * \param other Other ShaderSource object.
     * \return Reference to *this;
     */
    ShaderSource& operator=(ShaderSource&& other) & noexcept;

    /*!
     * \brief Swap with other ShaderSource object.
     * \param other Other ShaderSource object.
     */
    void swap(ShaderSource& other) noexcept;

    /*!
     * \brief Check, if loading was successful.
     * \return **false** on error.
     */
    bool valid() const noexcept;

    /*!
     * \brief Check, if loading was successful.
     * \return **true** on error.
     */
    bool operator!() const noexcept;

    /*!
     * \brief Get source of shader.
     * \return Source.
     */
    const std::string& source() const noexcept;

    /*!
     * \brief Get pointer-pointer to source.
     * \return pointer.
     */
    const char** sourcePtr() const noexcept;

    /*!
     * \brief Get number of files read.
     * \return Number of files.
     */
    int numFiles() const noexcept;

    /*!
     * \brief Get names of shader file and all included files.
     * \return Vector with filenames.
     */
    const std::vector<std::string> filenames() const noexcept;

    /*!
     * \brief Add define to code.
     *        Note: New define is placed after previously
     *        added defines.
     * \param defines Comma separated list of defines
     *               (e.g. "FOO_RADIUS 5, BAR")
     */
    void addDefines(const std::string& defines);

    /*!
     * \brief Add define to code.
     *        Note: New define is placed after previously
     *        added defines.
     * \param defines List of defines.
     */
    void addDefines(std::initializer_list<std::string> defines);

    /*!
     * \brief Get defines added to code.
     * \return List of defines.
     */
    const std::vector<std::string>& getDefines() const noexcept;


private:
    std::string                 m_code;
    std::vector<std::string>    m_filenames;
    bool                        m_valid;
    const char*                 m_srcPtr;
    std::vector<std::string>    m_defines;
};


} // namespace gl


#endif // PKOMMON_GL_SHADERSOURCE_HPP
