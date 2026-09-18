#pragma once

class ThrowsRuntime {
public:
    /**
     * @brief TBD: Describe declared.
     * @param[in] x Input parameter.
     * @return Return value.
     */
    int declared(int x);
    /**
     * @brief TBD: Describe defined.
     * @param[in] x Input parameter.
     * @return Return value.
     * @throws std::runtime_error std::runtime_error if an error occurs.
     * @details Calls: std::runtime_error().
     */
    int defined(int x) { if (x < 0) throw std::runtime_error("bad"); return x; }
};
