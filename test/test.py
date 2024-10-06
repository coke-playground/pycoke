import asyncio
import pycoke
import pathlib
import time

def line():
    print('-'*60)

async def test_value():
    for ival in [-1, 0, 1, 2147483647]:
        oval = await pycoke.return_int(ival)
        print(f'TestValue input:{ival} output:{oval}')

    for ival in ['', 'abcd', '去吃饭吧']:
        oval = await pycoke.return_string(ival)
        print(f'TestValue input:{ival} output:{oval}')

async def test_sleep():
    for sec in [0.123, 0.234, 0.345]:
        start = time.time()
        await pycoke.sleep(sec)
        cost = time.time() - start
        print(f'TestSleep {sec} cost {cost:.6f}')

async def test_exception():
    for i in range(3):
        try:
            await pycoke.throw_exception(i)
        except Exception as e:
            print(f'TestException type:{type(e)} {e}')


async def test_cancel():
    async def inner():
        # Although canceled due to timeout, the background task is still running
        # and needs to wait before the process exits
        await pycoke.sleep(2)

    try:
        await asyncio.wait_for(inner(), 0.5)
    except asyncio.TimeoutError:
        print('TestCancel timeout')


async def test_complex_work(url: str, filepath: str):
    p = pathlib.Path(filepath)
    if p.exists():
        print(f'{filepath} already exists')
        return

    try:
        await pycoke.complex_work(url, filepath)
    except pycoke.TaskException as e:
        print(f'TaskException {e}')
        return

    if not p.exists():
        print(f'{filepath} not exists')
        return

    def try_decode(url: bytes) -> str:
        try:
            return url.decode()
        except UnicodeDecodeError:
            return url

    with open(filepath, 'rb') as urls:
        for url in urls:
            print(try_decode(url.rstrip(b'\n')))

    # remove url file
    p.unlink()


async def test_queue():
    que = pycoke.StrQueue(2)

    try:
        # start background worker
        fut = asyncio.ensure_future(pycoke.do_work(que))

        # send data to background worker through que
        for i in range(5):
            data = f'value-{i}'
            if que.try_push_back(data):
                print(f'TryPush {data} done')
            else:
                await que.push_back(data)
                print(f'Push {data} done')
    finally:
        # close queue to notify worker to exit
        que.close()

    await fut


async def main():
    await test_value()
    await test_sleep()
    await test_exception()
    line()

    await test_complex_work('https://news.baidu.com/', 'a.url.txt')
    line()

    await test_complex_work('http://no.such.url/', 'b.url.txt')
    line()

    await test_queue()
    line()

    await test_cancel()


if __name__ == '__main__':
    asyncio.run(main())

    # wait all backgroud tasks before exit
    pycoke.wait()
