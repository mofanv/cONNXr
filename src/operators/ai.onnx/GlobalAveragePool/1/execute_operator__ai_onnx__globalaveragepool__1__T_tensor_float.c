//this file was generated by ../../../../../../scripts/onnx_generator/OperatorTemplate.py
#include "operator__ai_onnx__globalaveragepool__1.h"
#include "tracing.h"
#include "utils.h"

operator_status
execute_operator__ai_onnx__globalaveragepool__1__T_tensor_float(
    node_context *ctx
)
{
    TRACE_ENTRY(1);

    TRACE_NODE(2, true, ctx->onnx_node);

    /* UNCOMMENT AS NEEDED */

    Onnx__TensorProto *i_X = searchInputByName(ctx, 0);

    TRACE_TENSOR(2, true, i_X);

    // context_operator__ai_onnx__globalaveragepool__1 *op_ctx = ctx->executer_context;

    Onnx__TensorProto *o_Y = searchOutputByName(ctx, 0);

    TRACE_TENSOR(2, true, o_Y);

    /* DO CALCULATION HERE */

    // D1 x D2 x ...
    int64_t cardinality = 1;
    for (int i = 2; i < i_X->n_dims; i++) {
      cardinality *= i_X->dims[i];
    }

    o_Y->float_data = malloc(o_Y->n_float_data * sizeof(float));
    for (int n = 0; n < i_X->dims[0]; n++) {
        for (int c = 0; c < i_X->dims[1]; c++) {
            int offset = n*i_X->dims[1]*cardinality + c*cardinality;
            float sum = 0;
            for (int i = 0; i < cardinality; i++) {
                sum += i_X->float_data[offset+i];
            }
            o_Y->float_data[n*i_X->dims[1] + c] = sum / cardinality;
        }
    }

    TRACE_EXIT(1);

    /* CHANGE RETURN CODE IF THIS EXECUTER IS VALID */
    // return OP_ENOSYS;
    return OP_OK;
}