#pragma once
#include "Foundation/NonCopyable.h"
#include "GlobalShaderParam.hpp"

struct RenderObject;
struct IMaterialBatchDraw : private NonCopyable {
    virtual void Draw(std::span<RenderObject *const> batch, const GlobalShaderParam &globalShaderParam) = 0;
    virtual ~IMaterialBatchDraw() = default;
};

class ForwardPass : private NonCopyable {
public:
    void DrawBatchList(const std::vector<RenderObject *> &batchList, const GlobalShaderParam &globalShaderParam);
public:
    /**
     * \brief 
     * \param materialTypeName material type name
     * \param pMaterialBatchDraw This material corresponds to the object on which the draw is performed
     * \return material ID
     */
    static auto RegisterMaterialBatchDraw(std::string_view materialTypeName,
        std::unique_ptr<IMaterialBatchDraw> pMaterialBatchDraw) -> uint16_t;

    struct MaterialBatchDrawItem {
        std::string_view materialTypeName;
        std::unique_ptr<IMaterialBatchDraw> pMaterialBatchDraw;
    };
    static std::vector<MaterialBatchDrawItem> sMaterialBatchDrawItems;
};
