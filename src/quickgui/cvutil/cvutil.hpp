#ifndef QUICKGUI_OPENCV_CVUTIL_HPP_
#define QUICKGUI_OPENCV_CVUTIL_HPP_

#include <c4/error.hpp>
#include <c4/substr.hpp>
#include <quickgui/imgview.hpp>

C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wsign-conversion")
C4_SUPPRESS_WARNING_GCC_CLANG("-Wconversion")
C4_SUPPRESS_WARNING_GCC("-Wuseless-cast")
C4_SUPPRESS_WARNING_GCC("-Wfloat-equal")
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
C4_SUPPRESS_WARNING_GCC_CLANG_POP

namespace quickgui {

struct cvtypespecs
{
    int val;
    imgview::data_type_e data_type;
    uint32_t num_channels;
    const char* str;
};

cvtypespecs const& cvtype_lookup(int cvtypeint);
cvtypespecs const& cvtype_lookup(imgview const& view);
template<class T>
cvtypespecs cvtype_lookup() noexcept
{
    if constexpr(std::is_same_v<T, uint8_t>)
        return cvtype_lookup(CV_8U);
    else if constexpr(std::is_same_v<T, int8_t>)
        return cvtype_lookup(CV_8S);
    else if constexpr(std::is_same_v<T, uint16_t>)
        return cvtype_lookup(CV_16U);
    else if constexpr(std::is_same_v<T, int16_t>)
        return cvtype_lookup(CV_16S);
    else if constexpr(std::is_same_v<T, uint32_t>)
        C4_STATIC_ERROR(T, "invalid type");
    else if constexpr(std::is_same_v<T, int32_t>)
        return cvtype_lookup(CV_32S);
    else if constexpr(std::is_same_v<T, float>)
        return cvtype_lookup(CV_32F);
    else if constexpr(std::is_same_v<T, double>)
        return cvtype_lookup(CV_64F);
    else
        C4_STATIC_ERROR(T, "invalid type");
    return {};
}

c4::csubstr cvtype_str(int cvtypeint);
size_t cvtype_bytes(int cvtypeint);
int cvtype_to_video(int cvtypeint);
bool same_mat(cv::Mat const& mat, imgview const& view);
cv::Mat cvmat(imgview const& view);
cv::Mat cvmat(wimgview const& view);
imgview cvmat_to_imgview(const cv::Mat& mat);
wimgview cvmat_to_imgview(cv::Mat& mat);

} // namespace quickgui

#endif /* QUICKGUI_OPENCV_CVUTIL_HPP_ */
