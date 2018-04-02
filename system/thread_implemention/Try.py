def findCombine(strings, target):
    """

    :param strings: list[str]
    :param target: str
    :return:
    """
    def isSubString(s, target):
        if s in target:
            return True
        return False

    def dfs(a, tmpStrs):
        cnt = 0
        for i in range(len(tmpStrs)):
            if isSubString(a + tmpStrs[i], target):
                tmpSs = tmpStrs[:i].extend(tmpStrs[i + 1:])
                cnt += dfs(a + tmpStrs[i], tmpSs) + 1
        return cnt
    cnt = 0
    for i in range(len(strings)):
        if isSubString(strings[i], target):
            tmpStrs = strings[:i].extend(strings[i + 1:])
            cnt += dfs(strings[i], tmpStrs) + 1
    return cnt



