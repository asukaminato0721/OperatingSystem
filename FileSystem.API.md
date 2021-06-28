# FileSystem.h

    实现文件系统内部逻辑，调用Driver.h，提供以下接口供上级使用

## 重要函数

### FCBIndex Find(FCBIndex dir, string filename)

    在给定目录下查找某个文件的FCB号

- `FCBIndex dir` 查找的父目录
- `string filename` 待查找的目录名
- `FCBIndex 返回值` `filename`对应的文件 FCB 号。若没有找到这个文件，则返回`-1`

### FCBIndex Create(string name, FCBIndex dir, enum FileType t)

    在某目录下创建新文件/新目录

- `string name` 新文件/目录的名称
- `FCBIndex dir` 父目录的 FCB 号
- `enum FileType t` 枚举值，指明创建的是文件（`FileType::File`）还是目录（`FileType::Directory`）
- `FCBIndex 返回值` 新建文件的 FCB 号，若创建失败，则返回`-1`

### int64_t ReadFile(FCBIndex file, int64_t pos, int64_t len, uint8_t \*buff)

    将某文件的部分内容读取到缓冲区中

- `FCBIndex file` 待读取的文件 FCB 号
- `int64_t pos` 读取开始位置相对于文件开头的偏移量。即从文件`pos`字节处开始读取。这个数值不应该超过文件大小
- `int64_t len` 读取长度，以字节为单位。`pos + len`不应该超过文件大侠
- `uint8_t * buff` 接收缓冲区。缓冲区长度必须大于`len`，否则会出现溢出
- `int64_t 返回值` 读取到最后一个字节的下一位的偏移量，即`pos + len`。若读取失败，则返回`-1`

### int64_t WriteFile(FCBIndex file, int64_t pos, int64_t len, uint8_t \*buff)

    从缓存区将数据写入文件中

- `FCBIndex file` 待写入的文件 FCB 号
- `int64_t pos` 写入开始位置相对于文件开头的偏移量。即从文件`pos`字节处开始写入。
- `int64_t len` 写入长度，以字节为单位。
- `uint8_t * buff` 写入缓冲区，在缓冲区内放入待写入的数据。缓冲区长度必须大于`len`，否则会出现溢出
- `int64_t 返回值` 写入的最后一个字节的下一位的偏移量，即`pos + len`。若写入失败，则返回`-1`

### void FileInfo(FCBIndex file, FileControlBlock \*fcb)

    通过FCB号获取文件信息

- `FCBIndex file` 待查找的文件 FCB 号
- `FileControlBlock *fcb` 接收缓冲区。该文件的 FCB 结构体会被读取到`fcb`中，供后续使用

### vector\<FCBIndex\> GetChildren(FCBIndex dir)

    搜索某目录下所有文件的FCB号

- `FCBIndex dir` 待搜索目录的 FCB 号
- `vector<FCBIndex> 返回值` 该目录下所有文件的 FCB 号列表

## 结构体、枚举和类

### struct FileControlBlock

    文件控制块（FCB）结构体
    记录了文件的所有元数据
    可以使用FCB号索引得到，但不能根据结构体反推其FCB号

- `char Name[32]` 文件名
- `enum FileType Type` 文件类型
- `uint32_t Size` 文件大小
- `time_t CreateTime, ModifyTime, ReadTime` 创建时间、修改时间和最近读取时间
- `uint8_t AccessMode` 访问模式
- `FCBIndex Parent` 父目录的 FCB 号。根目录的 FCB 号永远为`0`
- `BlockIndex DirectBlock[10]` `10`个直接索引块，若不使用，则置为`-1`
- `BlockIndex Pointer` `1` 个一级间接索引块，若不使用，则置为`-1`
- `uint8_t __padding[6]` 字节填充区，将`FileControlBlock`扩充为`128`字节，无实际作用

### struct SuperBlock

    超级块的结构体
    记录了磁盘的所有元数据
    超级快在磁盘格式化是生成，并不再改变

- `uint16_t FCS` 循环校验码
- `uint32_t DiskSize` 磁盘大小(以字节为单位)
- `uint32_t BlockSize` 块大小(以字节为单位)
- `uint32_t BlockNum` 块数量(包括了所有块的数量)
- `BlockIndex FCBBitmapOffset` FCB 的位示图列表开始块号
- `BlockIndex DataBitmapOffset` Data 的位示图数据块开始块号
- `BlockIndex FCBOffset` FCB 列表开始块号
- `BlockIndex DataOffset` Data 数据块开始块号
- `uint32_t FCBNum, DataBlockNum` FCB 数量和 DataBlock 数量

### struct Block

    存储块的结构体
    在本系统中，存储块被分为5类：Super块，FCB位示图块，Data位示图块，FCB块，数据块。
    前4中数据块使用其他数据结构描述，而该结构体用于描述数据块。
    数据块包含了3总类型（Pointer，Dir和Data），分别用于存储一级索引、文件目录和二进制文件

- `uint16_t FCS` 循环校验码
- `uint16_t Size` 块内有效数据的长度
- `T data[]` 块内存储的数据。Pointer 块中`T = BlockIndex`，Dir 块中`T = FCBIndex`，Data 块中`T = uint8_t`

### static class Access

    访问模式，长度为8bit（1字节）
    各项访问模式之间互不冲突，以按位与( | )的方式连接

- `uint8_t None = 0` 无权限
- `uint8_t Read = 1 << 0` 可以读取
- `uint8_t Write = 1 << 1` 可以写入
- `uint8_t Delete = 1 << 2` 可以删除

### enum class FileType : uint8_t

    文件类型，用于指明该FCB对应的是文件还是目录

- `File` 该 FCB 对应的是文件
- `Directory` 该 FCB 对应的是目录
