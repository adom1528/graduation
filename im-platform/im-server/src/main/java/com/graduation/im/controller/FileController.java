package com.graduation.im.controller;

import com.graduation.im.common.Result;
import io.minio.MinioClient;
import io.minio.PutObjectArgs;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.multipart.MultipartFile;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

import java.util.UUID;

@Slf4j
@RestController
@RequestMapping("/file")
public class FileController {

    @Autowired
    private MinioClient minioClient;

    @Value("${minio.endpoint}")
    private String endpoint;

    @Value("${minio.bucket-name}")
    private String bucketName;

    @PostMapping("/upload")
    public Result<String> upload(@RequestParam("file") MultipartFile file) {
        try {
            // 1. 提取文件后缀名 (例如 .jpg, .png)
            String originalFilename = file.getOriginalFilename();
            if (originalFilename == null || !originalFilename.contains(".")) {
                return Result.error("文件格式不正确");
            }
            String extension = originalFilename.substring(originalFilename.lastIndexOf("."));

            // 2. 生成极其安全的雪花/UUID文件名 (防止不同用户上传同名文件导致覆盖)
            String newFileName = UUID.randomUUID().toString().replace("-", "") + extension;

            // 如果前端没传 Content-Type，默认把它当成普通的二进制文件流！
            String contentType = file.getContentType();
            if (contentType == null || contentType.isEmpty()) {
                contentType = "application/octet-stream"; // 终极保底类型
                log.warn("未能获取到文件的 Content-Type，已使用默认类型进行兜底");
            }

            // 中文防乱码处理：将中文名转成安全的 URL 编码格式
            String encodedFileName = URLEncoder.encode(originalFilename, StandardCharsets.UTF_8.toString()).replaceAll("\\+", "%20");

            // 2. 组装终极 Content-Disposition 字符串
            // 语法解析：
            // attachment: 告诉浏览器“千万别直接在网页里打开，给我弹窗下载！”
            // filename="...": 兼容老浏览器的写法
            // filename*=utf-8''...: 现代浏览器标准，完美支持中文！
            String contentDisposition = "attachment; filename=\"" + originalFilename + "\"; filename*=utf-8''" + encodedFileName;

            // 3. 准备 Header 集合
            Map<String, String> headers = new HashMap<>();
            headers.put("Content-Disposition", contentDisposition);

            // 4. 执行物理上传动作！
            minioClient.putObject(
                    PutObjectArgs.builder()
                            .bucket(bucketName)
                            .object(newFileName)
                            .stream(file.getInputStream(), file.getSize(), -1)
                            .contentType(contentType) // 传入经过保底处理的类型
                            .headers(headers)
                            .build()
            );

            // 4. 拼接极其优雅的绝对访问路径
            // 形如: http://localhost:19000/im-chat/8a7b6c...jpg
            String fileUrl = endpoint + "/" + bucketName + "/" + newFileName;

            log.info("文件上传成功: {}", fileUrl);
            return Result.success(fileUrl);

        } catch (Exception e) {
            log.error("文件上传失败", e);
            return Result.error("文件上传失败：" + e.getMessage());
        }
    }
}