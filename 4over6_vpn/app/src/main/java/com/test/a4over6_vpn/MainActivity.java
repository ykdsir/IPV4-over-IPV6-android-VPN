package com.test.a4over6_vpn;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText("???");

        //TODO 检查网络状态，获取IPV6地址

        //TODO 开启后台线程，调用startBackground()

        //TODO 开启前台计时器刷新界面

        //创建IP信息管道
        File extDir = Environment.getExternalStorageDirectory();
        Log.d("ykd",extDir.toString());
        File ipTunnel = new File(extDir,"ip_pipe");
        FileOutputStream fileOutputStream = null;
        try {
            fileOutputStream = new FileOutputStream(ipTunnel);
            BufferedOutputStream out = new BufferedOutputStream(fileOutputStream);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        //TODO 创建流量信息管道 info_pipe ，开启VPN服务，将安卓虚接口写入IP信息管道
//        out.write(arr, 0, arr.length)
//        out.flush();
//        out.close();
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native String startBackground();
    public native String test();
}
