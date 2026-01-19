package com.graduation.auth.common;

import lombok.Data;

@Data
public class Result<T> {
    private Integer code; // 状态码 200成功，500失败
    private String message; // 提示信息
    private T data; // 返回的数据

    public static <T> Result<T> success() {
        Result<T> result = new Result<>();
        result.code = 200;
        result.message = "success";
        return result;
    }

    public static <T> Result<T> success(T data) {
        Result<T> result = new Result<>();
        result.code = 200;
        result.message = "success";
        result.data = data;
        return result;
    }

    public static <T> Result<T> error(String msg) {
        Result<T> result = new Result<>();
        result.code = 500;
        result.message = msg;
        return result;
    }
}