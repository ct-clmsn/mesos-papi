public class FloatLoop {

  public static void main(String [] args) {
    float res = (float)0.0;

    for(int i = 0; i < 10000000; i++) {
      res += 1.0;
    }

    System.out.println(res);
  }
}

