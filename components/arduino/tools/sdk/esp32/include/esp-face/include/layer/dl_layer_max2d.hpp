#pragma once

#include "dl_constant.hpp"
#include "dl_variable.hpp"
#include "dl_tool.hpp"
#include "dl_nn_max2d.hpp"
#include "dl_layer_base.hpp"

namespace dl
{
    namespace layer
    {
        /**
         * @brief Max2D(input0, input1).
         * NOTE: maximum is element-wise, i.e., output[i,j,k] = max(input0[i,j,k], input1[i,j,k])
         * 
         * @tparam feature_t supports int16_t and int8_t,
         *         - int16_t: stands for operation in int16_t quantize
         *         - int8_t: stands for operation in int8_t quantize
         */
        template <typename feature_t>
        class Max2D : public Layer
        {
        private:
            Tensor<feature_t> *output;  /*<! output ptr of max2d >*/
            bool inplace;               /*<! true: the output will store to input0
                                             false: the output will store to a seperate memeory >*/
        public:
            
            /**
             * @brief Construct a new Max2D object.
             * 
             * @param name            name of max2d
             * @param inplace         true: the output will store to input0
             *                        false: the output will store to a seperate memeory
             */
            Max2D(const char *name = NULL, bool inplace = false) : Layer(name)
            {
                this->inplace = inplace;
            }

            /**
             * @brief Destroy the Max2D object
             * 
             */
            ~Max2D() 
            {
                if ((!this->inplace) && (this->output != NULL))
                {
                    delete this->output;
                }
            }

            /**
             * @brief Update output shape and exponent
             * NOTE: input0.shape must equal to input1.shape.
             *       input0.exponent must equal to input1.exponent.
             * 
             * @param input0 as one input
             * @param input1 as another input
             */
            void build(Tensor<feature_t> &input0, Tensor<feature_t> &input1)
            {
                assert(input0.is_same_shape(input1));
                assert(input0.exponent == input1.exponent);

                if(!this->inplace)
                {
                    if(this->output != NULL)
                    {
                        this->output = new Tensor<feature_t>;
                    }
                    this->output->set_exponent(this->output_exponent);
                    this->output->set_shape(input0.shape);
                    this->output->free_element();
                }
                else
                    this->output = &input0;
            }

            /**
             * @brief Get the output
             * 
             * @return Tensor<feature_t>& Max2D result
             */
            Tensor<feature_t> &get_output()
            {
                return *this->output;
            }

            /**
             * @brief Call Max2D operation.
             * 
             * @param input0      as one input
             * @param input1      as another input
             * @param assign_core not effective yet
             * @return Max2D result
             */
            Tensor<feature_t> &call(Tensor<feature_t> &input0, Tensor<feature_t> &input1, const std::vector<int> &assign_core = CONFIG_DEFAULT_ASSIGN_CORE)
            {
                DL_LOG_LAYER_LATENCY_INIT();

                if(!this->inplace)
                {
                    DL_LOG_LAYER_LATENCY_START();
                    this->output->apply_element();
                    this->output->set_exponent(input0.exponent);
                    DL_LOG_LAYER_LATENCY_END(this->name, "apply");

                    DL_LOG_LAYER_LATENCY_START();
                    nn::max2d(*this->output, input0, input1, assign_core);
                    DL_LOG_LAYER_LATENCY_END(this->name, "max2d");
                }
                else
                {
                    DL_LOG_LAYER_LATENCY_START();
                    nn::max2d<true>(*this->output, input0, input1, assign_core);
                    DL_LOG_LAYER_LATENCY_END(this->name, "max2d");
                }

                return *this->output;
            }
        };
    } // namespace layer
} // namespace dl
